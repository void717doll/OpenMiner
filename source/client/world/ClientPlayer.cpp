/*
 * =====================================================================================
 *
 *  OpenMiner
 *
 *  Copyright (C) 2018-2020 Unarelith, Quentin Bazin <openminer@unarelith.net>
 *  Copyright (C) 2019-2020 the OpenMiner contributors (see CONTRIBUTORS.md)
 *
 *  This file is part of OpenMiner.
 *
 *  OpenMiner is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  OpenMiner is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenMiner; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * =====================================================================================
 */

#include <glm/gtc/matrix_transform.hpp>

#include "common/core/GameClock.hpp"
#include "common/core/Registry.hpp"
#include "common/core/input/GameKey.hpp"

#include "client/core/GameConfig.hpp"
#include "client/core/input/GamePad.hpp"
#include "client/graphics/Camera.hpp"
#include "client/network/ClientCommandHandler.hpp"
#include "client/world/ClientPlayer.hpp"
#include "client/world/ClientWorld.hpp"

#include "common/world/WorldCollision.hpp"
#include "common/world/BetaPlayerMovement.hpp"

ClientPlayer *ClientPlayer::s_instance = nullptr;

ClientPlayer::ClientPlayer(Camera &camera) : m_camera(camera) {
	m_cameraLocalPos = Vector3f{0.f, 0.f, 1.625f};

	updateCamera();

	m_camera.setDPosition(m_x + m_cameraLocalPos.x, m_y + m_cameraLocalPos.y, m_z + m_cameraLocalPos.z);
}

void ClientPlayer::turnH(float angle) {
	m_viewAngleH = fmodf(m_viewAngleH + angle, 360.f);
	if (m_viewAngleH >= 180.f) m_viewAngleH -= 360.f;
	if (m_viewAngleH < -180.f) m_viewAngleH += 360.f;

	if (!Config::isViewBobbingEnabled)
		updateCamera();
}

void ClientPlayer::turnViewV(float angle) {
	m_viewAngleV = std::max(std::min(m_viewAngleV + angle, 90.f), -90.f);

	if (!Config::isViewBobbingEnabled)
		updateCamera();
}

void ClientPlayer::updateCamera() {
	float viewAngleH = m_viewAngleH;
	float viewAngleV = m_viewAngleV;
	float viewAngleRoll = m_viewAngleRoll;

	if (Config::isViewBobbingEnabled)
		applyViewBobbing(viewAngleH, viewAngleV, viewAngleRoll);

	float ch = cosf(viewAngleH * math::DEG_TO_RADf);
	float sh = sinf(viewAngleH * math::DEG_TO_RADf);
	float cv = cosf(viewAngleV * math::DEG_TO_RADf);
	float sv = sinf(viewAngleV * math::DEG_TO_RADf);
	float cr = cosf(viewAngleRoll * math::DEG_TO_RADf);
	float sr = sinf(viewAngleRoll * math::DEG_TO_RADf);

	m_forwardDir = Vector3f{ch * cv, sh * cv, sv};
	m_camera.setDirection(m_forwardDir);
	m_camera.setUpVector(Vector3f{sh * sr - ch * sv * cr, -ch * sr - sh * sv * cr, cv * cr});
}

void ClientPlayer::clearInputs() {
	m_forwardInput = 0;
	m_strafeInput = 0;
	m_jumpPressed = false;
	m_isSneaking = false;
}

void ClientPlayer::processInputs() {
	m_forwardInput = float(GamePad::isKeyPressed(GameKey::Forward)) - float(GamePad::isKeyPressed(GameKey::Back));
	m_strafeInput = float(GamePad::isKeyPressed(GameKey::Left)) - float(GamePad::isKeyPressed(GameKey::Right));
	m_jumpPressed = GamePad::isKeyPressed(GameKey::Jump);
	m_isSneaking = GamePad::isKeyPressed(GameKey::Sneak);
}

void ClientPlayer::simulateMovement(const ClientWorld &world) {
	const Chunk *chunk = world.getChunkAtBlockPos(int(std::floor(m_physicsPosition.x)),
		int(std::floor(m_physicsPosition.y)), int(std::floor(m_physicsPosition.z)));
	if (!chunk || !chunk->isInitialized()) {
		m_velocity.reset(0, 0, 0);
		m_onGround = false;
		return;
	}
	DoubleBox body = m_hitbox;
	body.move(m_physicsPosition);
	auto environment = retro::movementEnvironment(world, body);
	bool wasGrounded = m_onGround;
	retro::acceleratePlayer(m_velocity, m_forwardInput, m_strafeInput, m_viewAngleH * math::DEG_TO_RAD,
		m_jumpPressed, m_isSneaking, wasGrounded, environment);
	Vector3d requested = m_velocity;
	Vector3d movement = retro::movePlayer(world, body, requested, wasGrounded, m_isSneaking);
	m_physicsPosition += movement;
	m_onGround = requested.z < 0 && requested.z != movement.z;
	bool horizontalCollision = requested.x != movement.x || requested.y != movement.y;
	if (requested.x != movement.x) m_velocity.x = 0;
	if (requested.y != movement.y) m_velocity.y = 0;
	if (requested.z != movement.z) m_velocity.z = 0;
	retro::dragPlayer(m_velocity, wasGrounded, horizontalCollision, environment);
	if (horizontalCollision && (environment.water || environment.lava)) {
		DoubleBox exit = body;
		exit.move(movement.x + m_velocity.x, movement.y + m_velocity.y, m_velocity.z + 0.6);
		bool loaded;
		auto obstacles = collision::collectObstacles(world, exit, {0, 0, 0}, 0, loaded, false);
		bool clear = loaded;
		for (const auto &obstacle : obstacles) clear &= !exit.intersects(obstacle.box);
		auto liquid = retro::movementEnvironment(world, exit);
		if (clear && !liquid.water && !liquid.lava) m_velocity.z = double(0.3f);
	}
}

void ClientPlayer::updatePosition(const ClientWorld &world) {
	u32 now = GameClock::getInstance().getTicks();
	m_physicsRemainder += std::min(now - m_lastPhysicsTime, u32(500));
	m_lastPhysicsTime = now;
	while (m_physicsRemainder >= 50) {
		m_previousPhysicsPosition = m_physicsPosition;
		simulateMovement(world);
		m_physicsRemainder -= 50;
	}
	double fraction = double(m_physicsRemainder) / 50;
	Vector3d renderPosition = m_previousPhysicsPosition + (m_physicsPosition - m_previousPhysicsPosition) * fraction;
	Player::setPosition(renderPosition.x, renderPosition.y, renderPosition.z);
	m_camera.setDPosition(m_x,m_y,m_z + m_cameraLocalPos.z);
	updateCamera();

	// Checking to block at camera position to enable specific effects
	const BlockState *blockState = world.getBlockState((int)m_camera.getDPosition().x, (int)m_camera.getDPosition().y, (int)m_camera.getDPosition().z);
	if (blockState && blockState->fogDepth() != 0) {
		GameConfig::currentScreenEffect = 1;
		GameConfig::fogDepth = blockState->fogDepth();
		GameConfig::fogColor = blockState->fogColor();
	}
	else {
		GameConfig::currentScreenEffect = 0;
	}

	// Sending PlayerChunkPosUpdate if needed
	if (!m_lastChunkPos.has_value() || m_lastChunkPos.value() != getCurrentChunk()) {
		world.client().sendPlayerChunkPosUpdate();
		m_lastChunkPos = getCurrentChunk();
	}
}

void ClientPlayer::setPosition(double x, double y, double z) {
	Player::setPosition(x,y,z);
	m_physicsPosition = {x,y,z};
	m_velocity.reset(0,0,0);
	m_lastPhysicsTime = GameClock::getInstance().getTicks();
	m_onGround = false;
	m_physicsRemainder = 0;
	m_previousPhysicsPosition = m_physicsPosition;
	m_camera.setDPosition(x,y,z + m_cameraLocalPos.z);
}

void ClientPlayer::applyViewBobbing(float &viewAngleH, float &viewAngleV, float &viewAngleRoll) {
	if (std::hypot(m_velocity.x, m_velocity.y) > 0.001 && m_onGround) {
		if (!m_isMoving) {
			m_movementStartTime = GameClock::getInstance().getTicks();
			m_isMoving = true;
		}
	}
	else {
		m_isMoving = false;
	}

	if (m_isMoving) {
		// float t = (float)(GameClock::getInstance().getTicks());
		float t = (float)(GameClock::getInstance().getTicks() - m_movementStartTime);
		viewAngleH += 0.4f * sinf(t / 150.f);
		viewAngleV += 0.4f * sinf(t / 75.f);
		viewAngleRoll += 0.3f * -sinf(t / 150.f);
	}
}

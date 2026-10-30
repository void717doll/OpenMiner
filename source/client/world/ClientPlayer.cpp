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

#include "client/audio/SoundManager.hpp"
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
	viewAngleRoll += hurtEffect() * 8;

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

void ClientPlayer::notifyDamage() {
	m_hurtUntil = GameClock::getInstance().getTicks() + 500;
	++m_damageSerial;
}

void ClientPlayer::showHit(const Vector3d &position, u16 damage) {
	m_hitPosition = position;
	m_hitDamage = damage;
	m_hitUntil = GameClock::getInstance().getTicks() + 650;
}

void ClientPlayer::addChatMessage(const std::string &message) {
	if (message.empty()) return;
	m_chatMessages.push_back(message);
	while (m_chatMessages.size() > 5) m_chatMessages.pop_front();
	m_chatUntil = GameClock::getInstance().getTicks() + 15000;
}

float ClientPlayer::hurtEffect() const {
	if (!m_hurtUntil) return 0;
	u32 now = GameClock::getInstance().getTicks();
	if (now >= m_hurtUntil) return 0;
	float remaining = float(m_hurtUntil - now) / 500.f;
	return std::sin(remaining * remaining * remaining * remaining * float(M_PI));
}

void ClientPlayer::pressJump() {
	if (!isCreative() || m_mounted || isNoclip()) return;
	u32 now = GameClock::getInstance().getTicks();
	if (m_lastJump && now - m_lastJump < 300) {
		m_flying = !m_flying;
		m_velocity.reset(0, 0, 0);
		m_lastJump = 0;
	} else m_lastJump = now;
}

void ClientPlayer::toggleNoclip() {
	if (!isCreative() || m_mounted) return;
	setNoclip(!isNoclip());
	if (isNoclip()) m_flying = true;
	m_velocity.reset(0, 0, 0);
	addChatMessage(isNoclip() ? "noclip on" : "noclip off");
}

void ClientPlayer::clearInputs() {
	m_forwardInput = 0;
	m_strafeInput = 0;
	m_jumpPressed = false;
	m_isSneaking = false;
}

void ClientPlayer::pauseMovement() {
	m_lastPhysicsTime = GameClock::getInstance().getTicks();
	m_physicsRemainder = 0;
	m_previousPhysicsPosition = m_physicsPosition;
}

void ClientPlayer::processInputs() {
	m_forwardInput = float(GamePad::isKeyPressed(GameKey::Forward)) - float(GamePad::isKeyPressed(GameKey::Back));
	m_strafeInput = float(GamePad::isKeyPressed(GameKey::Left)) - float(GamePad::isKeyPressed(GameKey::Right));
	m_jumpPressed = GamePad::isKeyPressed(GameKey::Jump);
	m_isSneaking = GamePad::isKeyPressed(GameKey::Sneak);
}

void ClientPlayer::simulateMovement(const ClientWorld &world) {
	if (!health()) {
		clearInputs();
		m_velocity.reset(0, 0, 0);
		return;
	}
	const Chunk *chunk = world.getChunkAtBlockPos(int(std::floor(m_physicsPosition.x)),
		int(std::floor(m_physicsPosition.y)), int(std::floor(m_physicsPosition.z)));
	if (!chunk || !chunk->isInitialized()) {
		m_velocity.reset(0, 0, 0);
		m_onGround = false;
		return;
	}
	DoubleBox body = m_hitbox;
	body.move(m_physicsPosition);
	if (isCreative() && m_flying) {
		double yaw = m_viewAngleH * math::DEG_TO_RAD;
		double length = std::max(1.0, std::hypot(double(m_forwardInput), double(m_strafeInput)));
		Vector3d requested{(std::cos(yaw) * m_forwardInput - std::sin(yaw) * m_strafeInput) * 0.5 / length,
			(std::sin(yaw) * m_forwardInput + std::cos(yaw) * m_strafeInput) * 0.5 / length,
			(double(m_jumpPressed) - double(m_isSneaking)) * 0.5};
		m_physicsPosition += isNoclip() ? requested : retro::movePlayer(world, body, requested, false, false);
		m_velocity.reset(0, 0, 0);
		m_onGround = false;
		return;
	}
	auto environment = retro::movementEnvironment(world, body);
	bool wasGrounded = m_onGround;
	retro::acceleratePlayer(m_velocity, m_forwardInput, m_strafeInput, m_viewAngleH * math::DEG_TO_RAD,
		m_jumpPressed, m_isSneaking, wasGrounded, environment);
	Vector3d requested = m_velocity;
	if (environment.web) {
		requested.x *= 0.25; requested.y *= 0.25; requested.z *= 0.05;
		m_velocity.reset(0, 0, 0);
	}
	Vector3d movement = retro::movePlayer(world, body, requested, wasGrounded, m_isSneaking);
	m_physicsPosition += movement;
	m_onGround = requested.z < 0 && requested.z != movement.z;
	bool horizontalCollision = requested.x != movement.x || requested.y != movement.y;
	if (requested.x != movement.x) m_velocity.x = 0;
	if (requested.y != movement.y) m_velocity.y = 0;
	if (requested.z != movement.z) m_velocity.z = 0;
	retro::dragPlayer(m_velocity, wasGrounded, horizontalCollision, environment);
	if (auto *sound = SoundManager::instance()) {
		if (environment.water) {
			m_stepDistance += movement.length();
			if (m_stepDistance > 1.6) { sound->play("default_water_footstep", 0.15f); m_stepDistance = 0; }
		} else if (m_onGround && !m_isSneaking && !environment.lava) {
			m_stepDistance += std::hypot(movement.x, movement.y);
			if (m_stepDistance > 1.666 || (!wasGrounded && requested.z < -0.1)) {
				int x = int(std::floor(m_physicsPosition.x)), y = int(std::floor(m_physicsPosition.y));
				const BlockState *ground = world.getBlockState(x, y, int(std::floor(m_physicsPosition.z - 0.2)));
				if (ground) sound->playBlock(*ground, BlockSound::Step, m_physicsPosition);
				m_stepDistance = 0;
			}
		}
	}

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

void ClientPlayer::setRidePosition(bool mounted, const Vector3d &position) {
	bool wasMounted = m_mounted;
	m_mounted = mounted;
	m_cameraLocalPos.z = mounted ? 0.85f : 1.625f;
	m_velocity = {0,0,0};
	m_onGround = false;
	m_flying = false;
	setNoclip(false);
	if (!mounted || !wasMounted) setPosition(position.x,position.y,position.z);
	else {
		m_previousPhysicsPosition = {x(),y(),z()};
		m_physicsPosition = position;
		m_physicsRemainder = 0;
		m_lastPhysicsTime = GameClock::getInstance().getTicks();
	}
	if (!mounted) m_dismountRequested = false;
}

void ClientPlayer::updatePosition(const ClientWorld &world) {
	u32 now = GameClock::getInstance().getTicks();
	m_physicsRemainder += std::min(now - m_lastPhysicsTime, u32(500));
	m_lastPhysicsTime = now;
	if (m_mounted) {
		m_physicsRemainder = std::min(m_physicsRemainder, u32(50));
		if (m_isSneaking && !m_dismountRequested) {
			world.client().sendPlayerDismount();
			m_dismountRequested = true;
		}
		if (!m_isSneaking) m_dismountRequested = false;
	}
	while (!m_mounted && m_physicsRemainder >= 50) {
		m_previousPhysicsPosition = m_physicsPosition;
		simulateMovement(world);
		// ledge and corner checks must follow each physics step, not their combined displacement.
		if (m_physicsPosition != m_previousPhysicsPosition) world.client().sendPlayerPosUpdate();
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
	m_stepDistance = 0;
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

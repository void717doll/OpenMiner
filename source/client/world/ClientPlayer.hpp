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
#ifndef CLIENTPLAYER_HPP_
#define CLIENTPLAYER_HPP_

#include <cmath>

#include <glm/glm.hpp>
#include <deque>

#include "common/world/Player.hpp"

class Camera;
class ClientWorld;

class ClientPlayer : public Player {
	public:
		ClientPlayer(Camera &camera);

		void turnH(float angle);
		void turnViewV(float angle);

		void updateCamera();

		void processInputs();
		void updatePosition(const ClientWorld &world);

		void clearInputs();
		void pressJump();
		void toggleNoclip();
		void pauseMovement();
		void setRidePosition(bool mounted, const Vector3d &position);
		bool isRidingMinecart() const { return m_mounted; }
		void notifyDamage();
		float hurtEffect() const;
		u32 damageSerial() const { return m_damageSerial; }
		void showHit(const Vector3d &position, u16 damage);
		u32 hitUntil() const { return m_hitUntil; }
		u16 hitDamage() const { return m_hitDamage; }
		const Vector3d &hitPosition() const { return m_hitPosition; }
		void addChatMessage(const std::string &message);
		const std::deque<std::string> &chatMessages() const { return m_chatMessages; }
		u32 chatUntil() const { return m_chatUntil; }

		float dirTargetedX() const { return m_forwardDir.x; }
		float dirTargetedY() const { return m_forwardDir.y; }
		float dirTargetedZ() const { return m_forwardDir.z; }

		static ClientPlayer &getInstance() { return *s_instance; }
		static void setInstance(ClientPlayer *instance) { s_instance = instance; }

		void setPosition(double x, double y, double z);
		void setCameraRoll(float angle) { m_viewAngleRoll = angle; updateCamera(); };

		Camera &camera() { return m_camera; }
		const Vector3d &physicsPosition() const { return m_physicsPosition; }
		bool onGround() const { return m_onGround; }

	private:
		bool m_mounted = false;
		bool m_dismountRequested = false;
		bool m_flying = false;
		u32 m_lastJump = 0;
		void simulateMovement(const ClientWorld &world);

		void applyViewBobbing(float &viewAngleH, float &viewAngleV, float &viewAngleRoll);

		static ClientPlayer *s_instance;

		Camera &m_camera;

		Vector3f m_forwardDir;

		Vector3f m_cameraLocalPos;

		Vector3d m_velocity{0, 0, 0};
		u32 m_hurtUntil = 0;
		u32 m_damageSerial = 0;
		Vector3d m_hitPosition{0, 0, 0};
		u16 m_hitDamage = 0;
		u32 m_hitUntil = 0;
		std::deque<std::string> m_chatMessages;
		u32 m_chatUntil = 0;
		Vector3d m_physicsPosition{0, 0, 0};
		u32 m_lastPhysicsTime = 0;
		float m_forwardInput = 0;
		float m_strafeInput = 0;
		bool m_jumpPressed = false;
		bool m_isSneaking = false;
		bool m_onGround = false;
		double m_stepDistance = 0;
		u32 m_physicsRemainder = 0;
		Vector3d m_previousPhysicsPosition{0, 0, 0};

		// For view bobbing only
		bool m_isMoving = false;
		u64 m_movementStartTime = 0;

		std::optional<Vector3i> m_lastChunkPos;
};

#endif // CLIENTPLAYER_HPP_

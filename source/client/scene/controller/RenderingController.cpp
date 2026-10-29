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
#include <glm/gtx/quaternion.hpp>

#include "common/core/Registry.hpp"
#include "common/core/GameClock.hpp"
#include "common/scene/component/AnimationComponent.hpp"
#include "client/scene/component/ItemVisualComponent.hpp"
#include "common/scene/component/DrawableDef.hpp"
#include "common/scene/component/PositionComponent.hpp"
#include "common/scene/component/RotationComponent.hpp"

#include "client/gui/InventoryCube.hpp"
#include "client/graphics/MobModel.hpp"
#include "client/graphics/ItemBillboard.hpp"
#include "client/scene/component/DrawableComponent.hpp"
#include "client/scene/controller/RenderingController.hpp"

void RenderingController::update(entt::registry &registry) {
	registry.view<DrawableDef>().each([&](auto entity, auto &definition) {
		DrawableComponent &drawable = registry.get_or_emplace<DrawableComponent>(entity);
		if (definition.mobModel()) {
			if (auto *mob = dynamic_cast<MobModel *>(drawable.drawable()))
				mob->configure(definition.mobModel(), definition.mobScale(), definition.mobAppearance());
			else
				drawable.setDrawable<MobModel>(definition.mobModel(), definition.mobScale(), definition.mobAppearance());
		}
		else {
			const InventoryCubeDef &cubeDef = definition.getInventoryCubeDef();
			Registry &definitions = Registry::getInstance();
			bool sprite = false;
			if (definitions.hasBlock(cubeDef.blockID)) {
				const BlockState &state = definitions.getBlockFromStringID(cubeDef.blockID).getState(0);
				sprite = state.drawType() == BlockDrawType::XShape || !state.inventoryImage().empty();
			}
			if (definitions.hasBlock(cubeDef.blockID) && !sprite) {
				InventoryCube &cube = drawable.setDrawable<InventoryCube>(cubeDef.size, true);
				cube.setOrigin(cubeDef.origin);
				cube.updateVertexBuffer(definitions.getBlockFromStringID(cubeDef.blockID));
			}
			else if (definitions.hasItem(cubeDef.blockID))
				drawable.setDrawable<ItemBillboard>(cubeDef.blockID, cubeDef.size * 2);
		}
		registry.remove<DrawableDef>(entity);
	});
}

void RenderingController::draw(entt::registry &registry, RenderTarget &target, RenderStates states) {
	registry.view<DrawableComponent, PositionComponent, RotationComponent>().each([&](auto entity, auto &drawable, auto &position, auto &rotation) {
		if (auto *mob = dynamic_cast<MobModel *>(drawable.drawable()))
			mob->updateMotion({position.x, position.y, position.z});
		Transformable transformable;
		if (!dynamic_cast<MobModel *>(drawable.drawable()) && registry.all_of<AnimationComponent>(entity)) {
			auto &visual = registry.get_or_emplace<ItemVisualComponent>(entity);
			u64 now = GameClock::getInstance().getTicks();
			Vector3d displayed = visual.update({position.x, position.y, position.z}, now);
			double phase = double(now) / 1000.0 + double(u32(entity) % 32);
			float lift = dynamic_cast<ItemBillboard *>(drawable.drawable()) ? 0.125f : 0;
			displayed.z += lift + 0.04 * (1 + std::sin(phase * 2));
			transformable.setPosition(float(displayed.x), float(displayed.y), float(displayed.z));
			transformable.getRotationTransform().rotateZ(float(now % 4000) * 0.09f);
		}
		else {
			transformable.setPosition((float)position.x, (float)position.y, (float)position.z);
			transformable.getRotationTransform().getMatrix() = glm::toMat4(rotation.quat);
		}

		RenderStates drawStates = states;
		drawStates.transform *= transformable.getTransform();
		drawable.draw(target, drawStates);
	});
}

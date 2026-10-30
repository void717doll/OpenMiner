#include "common/network/Network.hpp"
#include "common/world/MusicRecords.hpp"
#include "common/world/Dimension.hpp"
#include "server/network/Server.hpp"
#include "server/network/ServerCommandHandler.hpp"
#include "server/world/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"

void ServerWorld::stopJukebox(int x, int y, int z) {
	if (!m_server) return;
	Network::Packet packet;
	packet << Network::Command::Jukebox << m_dimension.id() << s32(x) << s32(y) << s32(z) << u8(0);
	m_server->server().sendToAllClients(packet);
}

void ServerWorld::useJukebox(int x, int y, int z, ServerPlayer &player) {
	if (!m_server || !player.isOnline() || !player.isReady() || !player.health() || player.dimension() != m_dimension.id()) return;
	auto *chunk = static_cast<ServerChunk *>(getChunkAtBlockPos(x, y, z));
	const auto *state = getBlockState(x, y, z);
	if (!chunk || !chunk->isInitialized() || !state || state->block().stringID() != "default:jukebox") return;
	auto *data = addBlockData(x, y, z, 1, 1);
	if (data->inventory.width() != 1 || data->inventory.height() != 1) return;
	const auto &stored = data->inventory.getStack(0, 0);
	if (stored.amount()) {
		if (!dropWorldItem({x + 0.5, y + 0.5, z + 1.1}, stored.stringID(), 1, stored.damage(), false)) return;
		data->inventory.clearStack(0, 0); chunk->setModified(true); stopJukebox(x, y, z); return;
	}
	if (player.heldItemSlot() < 0 || player.heldItemSlot() >= player.inventory().width()) return;
	ItemStack held = player.heldItemStack();
	u16 record = held.amount() ? held.item().getGroupValue("group:om_record") : 0;
	if (!record || record > musicRecords.size() || !m_server->consumeHeldItem(player, 1, player.clientID())) return;
	held.setAmount(1); data->inventory.setItemStack(0, 0, held); chunk->setModified(true);
	Network::Packet packet;
	packet << Network::Command::Jukebox << m_dimension.id() << s32(x) << s32(y) << s32(z) << u8(record);
	m_server->server().sendToAllClients(packet);
	m_server->sendPlayerMessage(player.clientID(), std::string("now playing: ") + musicRecords[record - 1].title);
}

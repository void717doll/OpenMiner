#ifndef LOCALSESSION_HPP_
#define LOCALSESSION_HPP_

#include <chrono>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

#include "common/network/Network.hpp"

namespace Network {
class LocalSession {
	public:
		static std::shared_ptr<LocalSession> create() {
			std::lock_guard<std::mutex> lock(s_mutex);
			if (s_session)
				s_session->close();
			s_session = std::make_shared<LocalSession>();
			return s_session;
		}

		static std::shared_ptr<LocalSession> connect() {
			std::lock_guard<std::mutex> registryLock(s_mutex);
			if (!s_session)
				return nullptr;
			std::lock_guard<std::mutex> lock(s_session->m_mutex);
			if (s_session->m_closed || s_session->m_hasClient)
				return nullptr;
			s_session->m_hasClient = true;
			return s_session;
		}

		bool sendToServer(const Packet &packet) { return send(m_toServer, packet); }
		bool sendToClient(const Packet &packet) { return send(m_toClient, packet); }

		bool receiveOnServer(Packet &packet) { return receive(m_toServer, packet); }
		bool receiveOnClient(Packet &packet) { return receive(m_toClient, packet); }

		bool waitOnClient(Packet &packet, std::chrono::milliseconds timeout) {
			std::unique_lock<std::mutex> lock(m_mutex);
			m_changed.wait_for(lock, timeout, [this] { return m_closed || !m_toClient.empty(); });
			return pop(m_toClient, packet);
		}

		void waitOnServer(std::chrono::milliseconds timeout) {
			std::unique_lock<std::mutex> lock(m_mutex);
			m_changed.wait_for(lock, timeout, [this] { return m_closed || !m_toServer.empty(); });
		}

		void close() {
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_closed = true;
			}
			m_changed.notify_all();
		}

		bool isClosed() const {
			std::lock_guard<std::mutex> lock(m_mutex);
			return m_closed;
		}

	private:
		using Queue = std::deque<std::vector<unsigned char>>;

		bool send(Queue &queue, const Packet &packet) {
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				if (m_closed)
					return false;
				const auto *data = static_cast<const unsigned char *>(packet.getData());
				queue.emplace_back();
				if (packet.getDataSize())
					queue.back().assign(data, data + packet.getDataSize());
			}
			m_changed.notify_all();
			return true;
		}

		bool receive(Queue &queue, Packet &packet) {
			std::lock_guard<std::mutex> lock(m_mutex);
			return pop(queue, packet);
		}

		bool pop(Queue &queue, Packet &packet) {
			if (queue.empty())
				return false;
			// rebuilding from bytes gives the receiver an independent read cursor.
			packet.clear();
			if (!queue.front().empty())
				packet.append(queue.front().data(), queue.front().size());
			queue.pop_front();
			return true;
		}

		inline static std::mutex s_mutex;
		inline static std::shared_ptr<LocalSession> s_session;

		mutable std::mutex m_mutex;
		std::condition_variable m_changed;
		Queue m_toServer;
		Queue m_toClient;
		bool m_hasClient = false;
		bool m_closed = false;
};
}

#endif // LOCALSESSION_HPP_

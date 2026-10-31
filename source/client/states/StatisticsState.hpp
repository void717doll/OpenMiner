#ifndef STATISTICSSTATE_HPP_
#define STATISTICSSTATE_HPP_

#include <array>
#include <memory>
#include <vector>
#include "client/gui/MenuWidget.hpp"
#include "client/states/InterfaceState.hpp"

class Client;
class ClientPlayer;
class StatisticsState : public InterfaceState {
	public:
		StatisticsState(Client &client, const ClientPlayer &player, DrawableState *parent);
		void onEvent(const SDL_Event &event) override;
		void update() override;
	private:
		struct Row { std::string name; std::array<std::string, 3> values; };
		void refresh();
		void updateRowLabels();
		int visibleRows() const;
		void updateWidgetPosition() override;
		void draw(RenderTarget &target, RenderStates states) const override;
		const ClientPlayer &m_player;
		MenuWidget m_tabs{3, 1};
		TextButton m_done;
		Text m_title;
		Text m_worldName;
		std::vector<std::unique_ptr<Text>> m_labels;
		Image m_background{"texture-menu_background"};
		RectangleShape m_panel;
		RectangleShape m_scrollbar;
		int m_scale = 1;
		std::vector<Row> m_rows;
		int m_tab = 0;
		int m_offset = 0;
		u64 m_revision = ~u64(0);
};

#endif

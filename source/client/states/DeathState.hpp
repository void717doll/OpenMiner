#ifndef DEATHSTATE_HPP_
#define DEATHSTATE_HPP_

#include "client/gui/MenuWidget.hpp"
#include "client/states/InterfaceState.hpp"

class ClientCommandHandler;
class ClientPlayer;

class DeathState : public InterfaceState {
	public:
		DeathState(ClientCommandHandler &client, ClientPlayer &player, DrawableState *parent = nullptr);
		void onEvent(const SDL_Event &event) override;
		void update() override;
	private:
		void updateWidgetPosition() override;
		void draw(RenderTarget &target, RenderStates states) const override;

		ClientCommandHandler &m_client;
		ClientPlayer &m_player;
		RectangleShape m_filter;
		Text m_title;
		Text m_scoreLabel;
		Text m_scoreValue;
		MenuWidget m_menu{1, 2};
		bool m_waiting = false;
};

#endif // DEATHSTATE_HPP_

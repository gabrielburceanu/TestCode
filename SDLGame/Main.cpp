#include "Board.h"
#include "Common.h"
#include "AssetMgr.h"
#include "GraphicsMgr.h"

//@TODO: put all this in a precompiled header
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#include <vector>
#include <memory>

#include <assert.h>

using namespace std;

const int SCREEN_WIDTH  = 755;
const int SCREEN_HEIGHT = 600;
namespace
{
	enum EGameState
	{ 
		EGS_WaitingToStartGame = 0,
		EGS_GameRunning,
		EGS_GameOver
	};
}
int main(int argc, char** argv)
{


	if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS) == -1)
	{
		cout << SDL_GetError() << endl;
		return 1;
	}

	// @TODO remove the hardcoded text position values
	GraphicsMgr gfxMgr("Diamond Mine", 100, 100, SCREEN_WIDTH, SCREEN_HEIGHT, 
						/*scoreX =*/50, 
						/*scoreY =*/80, 
						/*timeX =*/50,
						/*timeY =*/130,
						/*startGameX =*/255,
						/*startGameY =*/30,
						/*gameOverX =*/270,
						/*gameOverY =*/30
						);

	AssetMgr assetMgr(gfxMgr.getRenderer());
	gfxMgr.setAssetMgr(&assetMgr);

	unique_ptr<Board> pBoard;
	{
		 Board* boardPtr = new Board(	assetMgr.getNumGemTypes(),
										/*gemW =*/35,
										/*gemW =*/35,
										/*boardBoundsXMin =*/315,
										/*boardBoundsYMin =*/95,
										/*boardW =*/360,
										/*boardH =*/352,
										&assetMgr,
										&gfxMgr);
		 
		pBoard.reset(boardPtr);
	}
	gfxMgr.setBoard(pBoard.get());
	gfxMgr.generateTextTextures();

	

	//int iW, iH;
	//SDL_QueryTexture(textImg, NULL, NULL, &iW, &iH);

	assetMgr.playMusic();
	EGameState gameState = EGS_WaitingToStartGame;
	pBoard->setGameRunning(false);
	gfxMgr.setStartGameTextVisible(true);
	gfxMgr.setGameOverTextVisible(false);

	uint32_t currentTime_ms = SDL_GetTicks();

	bool quit = false;
	SDL_Event e;

	while(!quit)
	{
		uint32_t newTime_ms = SDL_GetTicks();
		float dt_ms = static_cast<float>(newTime_ms - currentTime_ms);
		const float kMaxFrameTime = 100.f;
		const float kMinFrameTime = 1.f; 
		dt_ms = dt_ms < kMinFrameTime ? kMinFrameTime : dt_ms;
		dt_ms = dt_ms > kMaxFrameTime ? kMaxFrameTime : dt_ms;
		currentTime_ms = newTime_ms;
		
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
				//If user closes the window
				case SDL_QUIT:
					quit = true; 
					break;
				//If user presses any key
				case SDL_KEYDOWN:
					switch (e.key.keysym.sym)
					{
						case SDLK_s:
							if (gameState == EGS_WaitingToStartGame)
							{
								gfxMgr.setStartGameTextVisible(false);
								pBoard->setGameRunning(true);
								gameState = EGS_GameRunning;
							}
							break;
						case SDLK_r:
							gfxMgr.setStartGameTextVisible(false);
							gfxMgr.setGameOverTextVisible(false);
							pBoard->init();
							pBoard->setGameRunning(true);
							gameState = EGS_GameRunning;
							break;
						case SDLK_ESCAPE:
							quit = true;
							break;
						case SDLK_0:
							gfxMgr.setDebugDraw(!gfxMgr.getDebugDraw());
							break;
					}
					break;
				//If user clicks the mouse
				case SDL_MOUSEBUTTONDOWN:
				{
					if (gameState == EGS_GameRunning)
					{
						pBoard->mouseEvent(e.button.x, e.button.y, true);
					}
					break;
				}
				case SDL_MOUSEBUTTONUP:
				{
					if (gameState == EGS_GameRunning)
					{
						pBoard->mouseEvent(e.button.x, e.button.y, false);
					}
					break;
				}
			}
		}
		gfxMgr.update(dt_ms);
		pBoard->update(dt_ms);
		
		if (pBoard->getSecondsLeft() == 0)
		{
			pBoard->setGameRunning(false);
			gameState = EGS_GameOver;
			gfxMgr.setGameOverTextVisible(true);
		}
		gfxMgr.render();
	}

	SDL_Quit();
	return 0;
}
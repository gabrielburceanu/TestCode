#include "GraphicsMgr.h"
#include "AssetMgr.h"
#include "Board.h"
#include "Common.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <sstream>

#include <assert.h>



using namespace std;

GraphicsMgr::GraphicsMgr(const char* title, 
						 int x, int y, 
						 int w, int h, 
						 int scoreX, int scoreY, 
						 int timeX, int timeY, 
						 int gameOverX, int gameOverY,
						 int startGameX, int startGameY) :
	m_bDebugDraw(false),
	m_pAssetMgr(nullptr),
	m_pBoard(nullptr),
	m_scoreX(scoreX),
	m_scoreY(scoreY),
	m_timeX(timeX),
	m_timeY(timeY),
	m_gameOverX(gameOverX),
	m_gameOverY(gameOverY),
	m_startGameX(startGameX),
	m_startGameY(startGameY),
	m_pScoreTex(nullptr),
	m_pTimeTex(nullptr),
	m_pGameOverTex(nullptr),
	m_pStartGameTex(nullptr),
	m_bStartGameTextVisible(false),
	m_bGameOverTextVisible(false)
{
	m_pWindow = SDL_CreateWindow(title, x, y, w, h, SDL_WINDOW_SHOWN);
	if (!m_pWindow)
	{
		std::cout << SDL_GetError() << std::endl;
		exit(1);
	}

	m_pRenderer = SDL_CreateRenderer(m_pWindow, -1,
									SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!m_pRenderer)
	{
		Utils::logSDLError("CreateRenderer");
		exit(1);
	}
	
}


GraphicsMgr::~GraphicsMgr(void)
{
	SDL_DestroyTexture(m_pStartGameTex);
	SDL_DestroyTexture(m_pGameOverTex);

	SDL_DestroyRenderer(m_pRenderer);
	SDL_DestroyWindow(m_pWindow);
}

void GraphicsMgr::generateTextTextures()
{
	assert (!m_pStartGameTex && !m_pGameOverTex); 
	
	static const SDL_Color color = { 0, 0, 0 };

	m_pStartGameTex = renderText("Press 's' to start the game.", EFontType::EFT_FREE_SANS_BIG, color);
	
	m_pGameOverTex = renderText("Press 'r' to play again.", EFontType::EFT_FREE_SANS_BIG, color);
}
void GraphicsMgr::renderTexture(SDL_Texture* tex, int x, int y, int w, int h)
{
	//Setup the destination rectangle to be at the position we want
	SDL_Rect dst;
	dst.x = x;
	dst.y = y;
	dst.w = w;
	dst.h = h;
	SDL_RenderCopy(m_pRenderer, tex, NULL, &dst);
}

void GraphicsMgr::renderTexture(SDL_Texture* tex, int x, int y)
{
	int w, h;
	SDL_QueryTexture(tex, NULL, NULL, &w, &h);
	renderTexture(tex, x, y, w, h);
}

static string itos(int i)
{
	stringstream s;
	s << i;
	return s.str();
}

void GraphicsMgr::update(float dt_ms)
{
	//@TODO: I shouldn't create strings in the loop since it might allocate memory per frame
	//@TODO: implement a text rendering system that doesn't render the text to a texture each frame

	string text = "Score: ";
	text += itos(m_pBoard->getScore());

	static const SDL_Color color = { 255, 255, 255 };
	if (m_pScoreTex)
	{
		SDL_DestroyTexture(m_pScoreTex);
	}
	m_pScoreTex = renderText(text.c_str(), EFontType::EFT_FREE_SANS_MEDIUM, color);
	
	if (m_pTimeTex)
	{
		SDL_DestroyTexture(m_pTimeTex);
	}
	(text = "Time left: ") += itos(m_pBoard->getSecondsLeft());
	m_pTimeTex = renderText(text.c_str(), EFontType::EFT_FREE_SANS_MEDIUM, color);

}

SDL_Texture* GraphicsMgr::renderText(const char* message, EFontType font, SDL_Color color)
{
	//We need to first render to a surface as that's what TTF_RenderText
	//returns, then load that surface into a texture
	SDL_Surface *surf = TTF_RenderText_Blended(m_pAssetMgr->getFont(font), message, color);
	if (surf == nullptr)
	{
		Utils::logSDLError("TTF_RenderText");
		return nullptr;
	}
	SDL_Texture *texture = SDL_CreateTextureFromSurface(m_pRenderer, surf);
	if (texture == nullptr)
	{
		Utils::logSDLError("CreateTexture");
	}
	//Clean up the surface and font
	SDL_FreeSurface(surf);
	
	return texture;
}

void GraphicsMgr::render()
{
	SDL_RenderClear(m_pRenderer);
	renderTexture(m_pAssetMgr->getBackgroundTex(), 0, 0);

	m_pBoard->render(m_pRenderer);
	
	renderTexture(m_pScoreTex, m_scoreX, m_scoreY);
	renderTexture(m_pTimeTex, m_timeX, m_timeY);

	if (m_bStartGameTextVisible)
	{
		renderTexture(m_pStartGameTex, m_startGameX, m_startGameY);
	}
	else if (m_bGameOverTextVisible)
	{
		renderTexture(m_pGameOverTex, m_gameOverX, m_gameOverY);
	}
	SDL_RenderPresent(m_pRenderer);
}
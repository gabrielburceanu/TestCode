#ifndef GRAPHICS_MGR_H
#define GRAPHICS_MGR_H

struct SDL_Texture;
struct SDL_Renderer;
struct SDL_Color;
struct SDL_Window;
struct SDL_Renderer;
class Board;
class AssetMgr;
enum class EFontType : unsigned int;

class GraphicsMgr
{
protected:
	SDL_Window*		m_pWindow;
	SDL_Renderer*	m_pRenderer;

	AssetMgr*	m_pAssetMgr;
	Board*	m_pBoard;

	bool m_bDebugDraw;
	
	SDL_Texture* m_pScoreTex;
	SDL_Texture* m_pTimeTex;
	SDL_Texture* m_pGameOverTex;
	SDL_Texture* m_pStartGameTex;

	int m_scoreX;
	int m_scoreY;

	int m_timeX;
	int m_timeY;

	int m_gameOverX;
	int m_gameOverY;

	int m_startGameX;
	int m_startGameY;

	bool m_bStartGameTextVisible;
	bool m_bGameOverTextVisible;

public:
	GraphicsMgr::GraphicsMgr(const char* title, 
		int x, int y, 
		int w, int h, 
		int scoreX, int scoreY, 
		int timeX, int timeY, 
		int gameOverX, int gameOverY,
		int startGameX, int startGameY);
	~GraphicsMgr();

	SDL_Renderer*	getRenderer()		{ return m_pRenderer; }
	void			renderTexture(SDL_Texture* tex, int x, int y, int w, int h);
	void			renderTexture(SDL_Texture* tex, int x, int y);
	SDL_Texture*	renderText(const char* message, EFontType, SDL_Color color);
	
	void	setAssetMgr(AssetMgr* assetMgr) { m_pAssetMgr = assetMgr; }
	void	setBoard(Board* board)			{ m_pBoard = board; }
	void	render();
	void	update(float dt_ms);

	void	setDebugDraw(bool value)	{ m_bDebugDraw = value; }
	bool	getDebugDraw() const		{ return m_bDebugDraw; }
	
	void	generateTextTextures();
	void	setStartGameTextVisible(bool visible) { m_bStartGameTextVisible = visible; }
	void	setGameOverTextVisible(bool visible) { m_bGameOverTextVisible = visible; }

};
#endif//GRAPHICS_MGR_H




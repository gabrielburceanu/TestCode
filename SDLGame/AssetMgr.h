#ifndef ASSET_MGR_H
#define ASSET_MGR_H

#include <vector>

struct _TTF_Font;
struct _Mix_Music;
struct Mix_Chunk;
struct SDL_Renderer;
struct SDL_Texture;

enum class EFontType: unsigned int
{
	EFT_FREE_SANS_BIG, 
	EFT_FREE_SANS_MEDIUM, 
	EFT_COUNT
};

class AssetMgr
{
private:
	struct FontInfo
	{
		const char* file;
		int size;
	};

	static const FontInfo s_fontInfo[EFontType::EFT_COUNT];

	std::vector<_TTF_Font*> m_fonts;
	std::vector<SDL_Texture*> m_gemTextures;
	SDL_Texture* m_bgTex;
	
	_Mix_Music* m_music;
	Mix_Chunk* m_moved;
	Mix_Chunk* m_wrong;
	Mix_Chunk* m_erased;
	

	void initImage(SDL_Renderer* renderer);
	void loadSprites(SDL_Renderer* renderer);
	
	void initAudio();
	void loadSounds();

	void initFonts();

	static SDL_Texture* loadTexture(const char* file, SDL_Renderer* ren);
public:
	AssetMgr(SDL_Renderer* renderer);
	~AssetMgr();

	_TTF_Font* getFont(EFontType type)			{ return m_fonts[static_cast<int>(type)]; }
	
	int							getNumGemTypes() const	{ return m_gemTextures.size(); }
	std::vector<SDL_Texture*>&	getGemTextures()		{ return m_gemTextures; }
	SDL_Texture*				getBackgroundTex()		{ return m_bgTex; }
	
	void	playMusic();
	void	playMovedSound();
	void	playWrongSound();
	void	playErasedSound();
};
#endif//ASSET_MGR_H


#include "AssetMgr.h"
#include "Common.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#include <iostream>

#include <assert.h>

using namespace std;
const AssetMgr::FontInfo AssetMgr::s_fontInfo[] = {	{"../data/fonts/FreeSans.ttf", 40}, 
													{"../data/fonts/FreeSans.ttf", 30} };
AssetMgr::AssetMgr(SDL_Renderer* renderer) :
	m_bgTex(nullptr),
	m_music(nullptr),
	m_moved(nullptr),
	m_wrong(nullptr),
	m_erased(nullptr)
{
	initImage(renderer);
	initAudio();
	initFonts();
}


AssetMgr::~AssetMgr(void)
{
	{//Release Fonts
		for (auto& font : m_fonts)
		{
			TTF_CloseFont(font);
		}
		TTF_Quit();
	}
	{//Release Audio
		Mix_FreeMusic(m_music);
		m_music = nullptr;

		Mix_FreeChunk(m_moved);
		m_moved = nullptr;

		Mix_FreeChunk(m_wrong);
		m_wrong = nullptr;

		
		Mix_FreeChunk(m_erased);
		m_erased = nullptr;
		
		Mix_Quit();
	}
	{//Release Image
		SDL_DestroyTexture(m_bgTex);
		for (auto& gemTex : m_gemTextures)
		{
			SDL_DestroyTexture(gemTex);
		}

		IMG_Quit();
	}
}

void AssetMgr::initAudio()
{
	if (Mix_OpenAudio(/*frequency =*/44100, MIX_DEFAULT_FORMAT, /*channels =*/2, /*bytes =*/2048) < 0)
	{
		cout << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << endl;
		exit(1);
	}
	loadSounds();
}

void AssetMgr::initFonts()
{
	if (TTF_Init() != 0)
	{
		Utils::logSDLError("TTF_Init");
		exit(1);
	}

	for(int i = 0; i < static_cast<int>(EFontType::EFT_COUNT); ++i)
	{
		TTF_Font* font = TTF_OpenFont(s_fontInfo[i].file, s_fontInfo[i].size);
		if (font == nullptr)
		{
			Utils::logSDLError("TTF_OpenFont");
			exit(1);
		}
		m_fonts.push_back(font);
	}

}
void AssetMgr::initImage(SDL_Renderer* renderer)
{
	int flags = IMG_INIT_JPG | IMG_INIT_PNG;
	if ((IMG_Init(flags) & flags) != flags)
	{
		Utils::logSDLError("IMG_Init");
		exit(1);
	}

	loadSprites(renderer);
}

void AssetMgr::loadSprites(SDL_Renderer* renderer)
{
	m_bgTex = loadTexture("../data/BackGround.jpg", renderer);

	const char* gemTexFilenames[] =	{	
		"../data/Red.png",
		"../data/Blue.png",
		"../data/Green.png",
		"../data/Yellow.png",
		"../data/Purple.png"
	};

	bool spritesLoaded = (m_bgTex != nullptr);

	int numGems = sizeof(gemTexFilenames) / sizeof(gemTexFilenames[0]);
	m_gemTextures.resize(numGems);

	for (size_t i = 0, n = m_gemTextures.size(); i < n; ++i)
	{
		m_gemTextures[i] = loadTexture(gemTexFilenames[i], renderer);
		spritesLoaded &= (m_gemTextures[i] != nullptr);
	}

	assert(m_gemTextures.size() > 0);
	assert(spritesLoaded);
}

SDL_Texture* AssetMgr::loadTexture(const char* file, SDL_Renderer* ren)
{
	assert(file);
	SDL_Texture *texture = IMG_LoadTexture(ren, file);
	if (!texture)		
	{
		Utils::logSDLError("LoadTexture");
	}
	return texture;
}

void AssetMgr::loadSounds()
{
	bool success = true;

	//Load music
	m_music = Mix_LoadMUS( "../data/sounds/music.wav" );
	if (!m_music)
	{
		cout << "Failed to load music! SDL_mixer Error: " << Mix_GetError() << endl;
		success = false;
	}

	//Load sound effects
	m_moved = Mix_LoadWAV( "../data/sounds/swap.wav" );
	if (!m_moved)
	{
		cout << "Failed to load <moved> sound effect! SDL_mixer Error: " << Mix_GetError();
		success = false;
	}

	m_wrong = Mix_LoadWAV( "../data/sounds/wrong.wav" );
	if (!m_wrong)
	{
		cout << "Failed to load <wrong> sound effect! SDL_mixer Error: " << Mix_GetError();
		success = false;
	}
	
	m_erased = Mix_LoadWAV( "../data/sounds/erase.wav" );
	if (!m_erased)
	{
		cout << "Failed to load <erased> sound effect! SDL_mixer Error: " << Mix_GetError();
		success = false;
	}

	if (!success)
	{
		cout << "Error loading sounds";
		exit(1);
	}
}
void	AssetMgr::playMusic()
{
	Mix_FadeInMusic( m_music, /*loops =*/-1, /* ms =*/1000);
}
void	AssetMgr::playMovedSound()
{
	Mix_PlayChannel( /*channel =*/-1, m_moved, /*loops =*/0 );
}
void	AssetMgr::playWrongSound()
{
	Mix_PlayChannel( /*channel =*/-1, m_wrong, /*loops =*/0 );
}
void AssetMgr::playErasedSound()
{
	Mix_PlayChannel( /*channel =*/-1, m_erased, /*loops =*/0 );
}

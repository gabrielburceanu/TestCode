#ifndef BOARD_H
#define BOARD_H
#include "Matrix.h"
#include <SDL_config.h>
#include <vector>
#include <assert.h>

struct SDL_Renderer;
class AssetMgr;
class GraphicsMgr;
namespace Utils
{
	struct Point;
};

static const int kBoardRows = 8;
static const int kBoardCols = 8;
static const int kBoardRowsPlusOne = kBoardRows + 1;
static const int kPixelsPerMeters = 45;
static const int kTotalTime_s = 60;

static_assert(kBoardRows > 1 && kBoardCols > 1, "Invalid number of rows or columns.");
class Board
{
private:
	static const int8_t kEmptyCellColor	= -1;
	static const int8_t kSwapCellColor	= -2;
	
	struct Cell
	{
		int8_t color;
	};
	typedef Matrix<Cell, kBoardRows, kBoardCols> GemsMatrix;
	
	class SwappingGemsPair
	{

		bool m_bReturning;
		int m_pairState;
		int m_gem1Idx;
		int m_gem2Idx;
	
	public:
		static const int kNotFinished = 0;
		static const int kOtherGemChained = 1;
		static const int kOtherGemDidntChain = 2;

		SwappingGemsPair(int gem1Idx, int gem2Idx) : 
			m_pairState(kNotFinished), 
			m_gem1Idx(gem1Idx), 
			m_gem2Idx(gem2Idx),
			m_bReturning(false)
		{
		}
		void setGem1Idx(int idx) { m_gem1Idx = idx; }
		void setGem2Idx(int idx) { m_gem2Idx = idx; }
		int getGem1Idx() const { return m_gem1Idx; }
		int getGem2Idx() const { return m_gem2Idx; }

		void setIsReturning() { m_bReturning = true; }
		bool isReturning() const { return m_bReturning; }

		void setHasChained(bool hasChained)
		{
			m_pairState = hasChained ? kOtherGemChained : kOtherGemDidntChain;
		}

		bool hasFinished() const { return m_pairState != kNotFinished;	}
		bool hasChained() const { return m_pairState == kOtherGemChained; }
	};
	std::vector<SwappingGemsPair> m_swappingGemPairs;

	//@TODO: SwappingGem and FallingGem could use a little more encapsulation
	
	struct SwappingGem
	{
		bool m_bMoving;

		bool	m_bAxisX;
		bool	m_bPositiveDir;
		float	m_speed;
		float	m_pos;
		int		m_otherAxisPos;

		int		m_finalPos;
		int		m_destRow;
		int		m_destCol;
		int		m_swapPairIdx;

		int8_t m_color;

		void invalidatePair() { m_swapPairIdx = -1; }
		bool hasValidPair() const { return m_swapPairIdx >= 0; }

		int x() const { return m_bAxisX ? static_cast<int>(m_pos) : m_otherAxisPos; }
		int y() const { return m_bAxisX ? m_otherAxisPos : static_cast<int>(m_pos); }

		SwappingGem(int startX, int startY, int destX, int destY, int swapPairIdx, int dest_row, int dest_col, int8_t color);
	};

	struct FallingGem
	{
		//bool m_bMoving;
		float m_speed;
		float m_posX;
		float m_posY;
		int8_t m_color;

		int x() const { return static_cast<int>(m_posX); }
		int y() const { return static_cast<int>(m_posY); }

		void init(int startX, int startY, int8_t color)
		{
			//m_bMoving = true;
			m_speed = 4.f * kPixelsPerMeters;
			m_posX = static_cast<float>(startX);
			m_posY = static_cast<float>(startY);
			m_color = color;
		}
	};
	


	std::vector<SwappingGem>	m_swappingGems;
	
	// m_fallingGems is a vector of kBoardCols ring buffers used to make sure we update the 
	// gems in order, without reallocating memory and without sorting them each frame
	std::vector<std::vector<FallingGem> >	m_fallingGems;
	std::vector<int> m_fallingGemsStartIdx;
	std::vector<int> m_fallingGemsEndIdx;

	enum EBoardState {EBS_FIRST_SELECTION, EBS_SECOND_SELECTION};
	EBoardState m_boardState;
	
	GemsMatrix mat;

	int m_numGemTypes;

	int m_gemW;
	int m_gemH;
	int m_boardW;
	int m_boardH;

	int m_boardBoundsXMin;
	int m_boardBoundsXMax;
	int m_boardBoundsYMin;
	int m_boardBoundsYMax;

	int m_paddingW;
	int m_paddingH;
	
	int m_tileSizeW;
	int m_tileSizeH;
	
	int		m_lastClickedRow;
	int		m_lastClickedCol;
	int8_t	m_lastClickedColor;
	
	int m_score;
	uint32_t m_time_ms;
		
	bool m_bGameRunning;

	bool solveBoardAtPos(int modifiedCellRow, int modifiedCellCol);
	void solveFallAtPos(int row, int col);

	void DrawGrid(SDL_Renderer* renderer);
	
	AssetMgr* m_pAssetMgr;
	GraphicsMgr* m_pGfxMgr;

	bool m_bPlayerHasMoved;
	//void	setAssetMgr(AssetMgr* assetMgr) { m_pAssetMgr = assetMgr; }
	//void	setGraphicsMgr(GraphicsMgr* gfxMgr) { m_pAssetMgr = assetMgr; }

	bool isCellEmpty(int row, int col) const { return mat(row, col).color == kEmptyCellColor; }
	bool isCellSwapping(int row, int col) const { return mat(row, col).color == kSwapCellColor; }
	bool isStaticGem(const Cell& crtCell) const { return crtCell.color != kEmptyCellColor && crtCell.color != kSwapCellColor; }

	int checkLineChain(int startRow, int startCol, bool axisX, bool positiveDir) const;

	void swapGems(int row1, int col1, int row2, int col2, bool addPair);
	void releasePair(int swapPairIdx);
	void removeSwappingGem(int idx);
	void updateSwappingGems(float dt_s);
	void updateFallingGems(float dt_s);

	void addFallingGem(int startX, int startY, int8_t color);
	bool solveSelectionValidity();

public:
	int getTileCenterX(int col) const;
	int getTileCenterY(int row) const;
	Utils::Point getTileCenter(int row, int col) const;
	
	int getRowByPos(int y) const;
	int getColByPos(int x) const;
	Utils::Point getCellByPos(int x, int y) const;


	void update(float dt_ms);
	void render(SDL_Renderer* renderer);
	void Board::mouseEvent(int x, int y, bool bMouseDown);
	Board(int numGemTypes, int gemW, int gemH, int boardBoundsXMin, int boardBoundsYMin, int boardW, int boardH, AssetMgr* pAssetMgr, GraphicsMgr* pGfxMgr);
	~Board();
	int getScore() const { return m_score; }
	int getSecondsLeft() const
	{
		int m_time_s = static_cast<int>(kTotalTime_s - ( m_time_ms * 0.001f));
		return m_time_s > 0 ? m_time_s : 0; 
	}
	void init();
	void setGameRunning(bool running) { m_bGameRunning = running; }
};
#endif//BOARD_H

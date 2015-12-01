#include "Board.h"
#include "GraphicsMgr.h"
#include "AssetMgr.h"
#include "Common.h"

#include <SDL.h>

#include <ctime>

#include <stdlib.h>

using namespace std;
using namespace Utils;


template <typename T>
static void removeElement(std::vector<T>& vect, int idx)
{
	//Note: order of the elements is changed
	//swap with the last one and release the last element

	assert(idx >= 0 && idx < static_cast<int>(vect.size()));
	int lastIdx = vect.size() - 1;
	if (lastIdx != idx)
	{
		vect[idx] = vect[lastIdx];
	}
	vect.pop_back();
}

Board::Board(int numGemTypes, int gemW, int gemH, int boardBoundsXMin, int boardBoundsYMin, int boardW, int boardH, 
				AssetMgr* pAssetMgr, GraphicsMgr* pGfxMgr) :
	m_pAssetMgr(pAssetMgr),
	m_pGfxMgr(pGfxMgr)
{
	m_numGemTypes = numGemTypes;

	m_gemW = gemW;
	m_gemH = gemH;
	m_boardW = boardW;
	m_boardH = boardH;

	m_boardBoundsXMin = boardBoundsXMin; 
	m_boardBoundsXMax = boardBoundsXMin + boardW;

	m_boardBoundsYMin = boardBoundsYMin; 
	m_boardBoundsYMax = boardBoundsYMin + boardH; 

	m_paddingW =  (boardW - kBoardCols * gemW) / kBoardCols;
	m_paddingH =  (boardH - kBoardRows * gemH) / kBoardRows;
	
	m_tileSizeW = gemW + m_paddingW;
	m_tileSizeH = gemH + m_paddingH;

	init();
}

void Board::init()
{
	m_boardState = EBS_FIRST_SELECTION;
	
	m_lastClickedRow	= -1;
	m_lastClickedCol	= -1;
	m_lastClickedColor	= -1;

	m_time_ms = 0;
	m_score = 0;
	
	m_swappingGemPairs.clear();
	m_swappingGems.clear();
	m_fallingGems.clear();
	for (size_t i = 0, n = m_fallingGems.size(); i < n; ++i)
	{
		m_fallingGems[i].clear();
	}
	m_fallingGemsStartIdx.clear();
	m_fallingGemsEndIdx.clear();

	//allocate some memory to avoid too many reallocations
	m_swappingGemPairs.reserve(5);
	m_swappingGems.reserve(10);
	m_fallingGems.resize(kBoardCols);

	for (size_t i = 0, n = kBoardCols; i < n; ++i)
	{
		m_fallingGems[i].resize(kBoardRowsPlusOne);
	}
	
	m_fallingGemsStartIdx.resize(kBoardRowsPlusOne);
	m_fallingGemsEndIdx.resize(kBoardRowsPlusOne);
	
	std::fill (m_fallingGemsStartIdx.begin(), m_fallingGemsStartIdx.end(), 0);
	std::fill (m_fallingGemsEndIdx.begin(), m_fallingGemsEndIdx.end(), 0);
	m_bPlayerHasMoved = false;

	srand((unsigned int)time(nullptr));
	//srand((unsigned int)0);
	for (int row = 0; row < kBoardRows; ++row)
	{
		for (int col = 0; col < kBoardCols; ++col)
		{
			bool eraseVertical = false;
			bool eraseHorizontal = false;
			do 
			{
				//@TODO: use a uniform distribution(not just here)
				mat(row, col).color = rand() % m_numGemTypes;

				//This can be optimized since we only need to know if the chain is 
				//longer than 3 gems, we don't need to know exactly how long it is
				int rowStart	= checkLineChain(row, col, /*axisX =*/false, /*positiveDir =*/ false);
				int colStart	= checkLineChain(row, col, /*axisX =*/true, /*positiveDir =*/ false);

				int verticalGemChain	= row - rowStart + 1; 
				int horizontalGemChain	= col - colStart + 1; 

				eraseVertical	= verticalGemChain	>= 3;
				eraseHorizontal	= horizontalGemChain >= 3;

			} while (eraseVertical || eraseHorizontal);
		}
	}

	//add from bottom to the top
	for (int row = kBoardRows - 1; row >= 0; --row)
	{
		for (int col = 0; col < kBoardCols; ++col)
		{
			Point pos = getTileCenter(row - 2 * kBoardRows - col, col);
			addFallingGem(col, pos.y, mat(row, col).color);
			mat(row, col).color = kEmptyCellColor;
		}
	}
}


Board::~Board()
{
}

int Board::checkLineChain(int startRow, int startCol, bool axisX, bool positiveDir) const
{
	int upperLimit = axisX ? kBoardCols : kBoardRows;
	int inc = positiveDir ? 1 : -1;
	int limit = axisX ? startCol : startRow;
	int8_t color = mat(startRow, startCol).color;
	
	while (true)
	{
		limit += inc;

		if (positiveDir ? limit >= upperLimit : limit < 0)
			break;

		Cell& cell = axisX ? mat(startRow, limit) : mat(limit, startCol);
		if (cell.color != color || !isStaticGem(cell))
		{
			break;
		}
	}
	limit -= inc;
	return limit;
}
void Board::solveFallAtPos(int checkedRow, int checkedCol)
{
	int positionsToFall = 0;
	for (int row = checkedRow + 1; row < kBoardRows; ++row)
	{
		if (mat(row, checkedCol).color != kEmptyCellColor)
			break;
		++positionsToFall;
	}
	
	if (positionsToFall == 0)
		return;

	for (int row = checkedRow; row >= -positionsToFall; --row)
	{
		int8_t color;
		if (row >= 0)
		{
			int8_t& gemColor = mat(row, checkedCol).color;
			if (gemColor == kEmptyCellColor || gemColor == kSwapCellColor)
			{
				break;
			}
			color = gemColor;
			gemColor = kEmptyCellColor;
		}
		else 
		{
			color = rand() % m_numGemTypes;
		}
		Point pos = getTileCenter(row, checkedCol);
		addFallingGem(checkedCol, pos.y, color);
	}
}
bool Board::solveBoardAtPos(int modifiedCellRow, int modifiedCellCol)
{
	int rowStart	= checkLineChain(modifiedCellRow, modifiedCellCol, /*axisX =*/false, /*positiveDir =*/ false);
	int rowEnd		= checkLineChain(modifiedCellRow, modifiedCellCol, /*axisX =*/false, /*positiveDir =*/ true);
	int colStart	= checkLineChain(modifiedCellRow, modifiedCellCol, /*axisX =*/true, /*positiveDir =*/ false);
	int colEnd		= checkLineChain(modifiedCellRow, modifiedCellCol, /*axisX =*/true, /*positiveDir =*/ true);
	
	int verticalGemChain	= rowEnd - rowStart + 1; 
	int horizontalGemChain	= colEnd - colStart + 1; 
	
	bool eraseVertical		= verticalGemChain	>= 3;
	bool eraseHorizontal	= horizontalGemChain >= 3;
	int score = 0;
	if (eraseVertical)
	{
		for (int row = rowStart; row <= rowEnd; ++row)
		{
			mat(row, modifiedCellCol).color = kEmptyCellColor;
		}
		//make the upper gems fall
		for (int row = rowStart - 1; row >= -verticalGemChain; --row)
		{
			int8_t color;
			if (row >= 0)
			{
				int8_t& gemColor = mat(row, modifiedCellCol).color;
				if (gemColor == kEmptyCellColor || gemColor == kSwapCellColor)
				{
					break;
				}
				color = gemColor;
				gemColor = kEmptyCellColor;
			}
			else 
			{
				color = rand() % m_numGemTypes;
			}
			Point pos = getTileCenter(row, modifiedCellCol);
			addFallingGem(modifiedCellCol, pos.y, color);
		}

		score += verticalGemChain;
	}
	
	if (eraseHorizontal)
	{
		for (int col = colStart; col <= colEnd; ++col)
		{
			mat(modifiedCellRow, col).color = kEmptyCellColor;
		}

		//make the upper gems fall
		for (int col = colStart; col <= colEnd; ++col)
		{
			if (eraseVertical && col == modifiedCellCol)
			{
				//this case has already been treated
				continue;
			}
			for (int row = modifiedCellRow - 1; row >= -1; --row)
			{
				int8_t color;
				if (row >= 0)
				{
					int8_t& gemColor = mat(row, col).color;
					if (gemColor == kEmptyCellColor || gemColor == kSwapCellColor)
					{
						break;
					}
					color = gemColor;
					gemColor = kEmptyCellColor;
				}
				else 
				{
					color = rand() % m_numGemTypes;
				}
				Point pos = getTileCenter(row, col);
				addFallingGem(col, pos.y, color);
			}
		}
		score += verticalGemChain;
	}

	if (eraseHorizontal && eraseVertical)
	{
		//this gives an extra point when having a cross pattern so substract it
		//and double the score as a bonus
		score = (score - 1) * 2;
	}

	bool bHasErased = eraseHorizontal || eraseVertical;
	if (bHasErased)
	{
		m_pAssetMgr->playErasedSound();
	}
	m_score += score; 
	
	return bHasErased;
}
bool Board::solveSelectionValidity()
{
	if (m_boardState == EBS_SECOND_SELECTION && mat(m_lastClickedRow, m_lastClickedCol).color != m_lastClickedColor)
	{
		//The selected gem's state has changed, unselect it
		m_boardState = EBS_FIRST_SELECTION;		
		return false;
	}
	return true;
}
void Board::mouseEvent(int x, int y, bool bMouseDown)
{
	if (x >= m_boardBoundsXMin && x < m_boardBoundsXMax &&
		y >= m_boardBoundsYMin && y < m_boardBoundsYMax)
	{	
		int colClicked = (x - m_boardBoundsXMin) / m_tileSizeW;
		int rowClicked = (y - m_boardBoundsYMin) / m_tileSizeH;
		
		if (!isStaticGem(mat(rowClicked, colClicked)))
			return;

		if (!solveSelectionValidity())
			return;
		
		if (m_boardState == EBS_FIRST_SELECTION && bMouseDown)
		{
			m_lastClickedRow	= rowClicked;
			m_lastClickedCol	= colClicked;
			m_lastClickedColor = mat(rowClicked, colClicked).color;
			m_boardState = EBS_SECOND_SELECTION;
		}
		else
		{
			int distRow = m_lastClickedRow - rowClicked;
			int distCol = m_lastClickedCol - colClicked;
			
			int distRowAbs = abs(distRow);
			int distColAbs = abs(distCol);

			int ManhattanDistanceOnBoard = distRowAbs + distColAbs;
			bool areNeighbors = (ManhattanDistanceOnBoard == 1); 
			bool isSelf = ManhattanDistanceOnBoard == 0;
			
			int rowToSwapWith = -1;
			int colToSwapWith = -1;

			if (areNeighbors && bMouseDown)
			{ 
				rowToSwapWith = rowClicked;
				colToSwapWith = colClicked;
			}
			else if (m_boardState == EBS_SECOND_SELECTION && !bMouseDown && !isSelf && (distRowAbs == 0 || distColAbs == 0))
			{
				//if I drag the mouse in a direction I should 
				//switch with the closest gem in that direction
				if (distRowAbs == 0)
				{
					int colOffset = (distCol > 0) ? -1 : 1;
					rowToSwapWith = m_lastClickedRow;
					colToSwapWith = m_lastClickedCol + colOffset;
				}
				else
				{
					int rowOffset = (distRow > 0) ? -1 : 1;
					rowToSwapWith = m_lastClickedRow + rowOffset;
					colToSwapWith = m_lastClickedCol;
				}
			}

			if (rowToSwapWith >= 0 && colToSwapWith >= 0 && isStaticGem(mat(rowToSwapWith, colToSwapWith)))
			{
				swapGems(m_lastClickedRow, m_lastClickedCol, rowToSwapWith, colToSwapWith, true);
				m_bPlayerHasMoved = true;
				m_pAssetMgr->playMovedSound();
			}

			// don't select and immediately unselect gem if clicked and released on the same gem
			if (bMouseDown || !isSelf)
			{
				m_boardState = EBS_FIRST_SELECTION;
				//@TODO: uncomment when I find a better sound
				//m_pAssetMgr->playWrongSound();
			}
		}
	}
}
void Board::swapGems(int row1, int col1, int row2, int col2, bool addPair)
{
	int8_t& colorGem1 = mat(row1, col1).color;
	int8_t& colorGem2 = mat(row2, col2).color;
	Point posGem1 = getTileCenter(row1, col1);
	Point posGem2 = getTileCenter(row2, col2);
	
	int pairIdx = -1;
	if (addPair)
	{
		pairIdx = static_cast<int>(m_swappingGemPairs.size());
		int gem1Idx = m_swappingGems.size();
		int gem2Idx = gem1Idx + 1;
		m_swappingGemPairs.push_back(SwappingGemsPair(gem1Idx, gem2Idx));
	}

	m_swappingGems.push_back(SwappingGem(posGem1.x, posGem1.y, posGem2.x, posGem2.y, pairIdx, row2, col2, colorGem1));
	m_swappingGems.push_back(SwappingGem(posGem2.x, posGem2.y, posGem1.x, posGem1.y, pairIdx, row1, col1, colorGem2));
	colorGem1 = kSwapCellColor;
	colorGem2 = kSwapCellColor;
}

void Board::releasePair(int swapPairIdx)
{
	{
		SwappingGemsPair& pair = m_swappingGemPairs[swapPairIdx];
		m_swappingGems[pair.getGem1Idx()].invalidatePair();
		m_swappingGems[pair.getGem2Idx()].invalidatePair();
	}
	int lastIdx = m_swappingGemPairs.size() - 1; 
	removeElement(m_swappingGemPairs, swapPairIdx);
	if (swapPairIdx != lastIdx && !m_swappingGemPairs[swapPairIdx].hasFinished())
	{
		SwappingGemsPair& pair = m_swappingGemPairs[swapPairIdx];
		m_swappingGems[pair.getGem1Idx()].m_swapPairIdx = swapPairIdx; 
		m_swappingGems[pair.getGem2Idx()].m_swapPairIdx = swapPairIdx; 
	}
}


void Board::removeSwappingGem(int idx)
{
	int lastIdx = m_swappingGems.size() - 1; 

	removeElement(m_swappingGems, idx);
	if (idx != lastIdx && m_swappingGems[idx].hasValidPair())
	{
		SwappingGemsPair& pair = m_swappingGemPairs[m_swappingGems[idx].m_swapPairIdx];
		if (pair.getGem1Idx() == lastIdx)
		{
			pair.setGem1Idx(idx);
		}
		else
		{
			pair.setGem2Idx(idx);
		}
	}
}
void Board::updateSwappingGems(float dt_s)
{
	for (size_t i = 0, n = m_swappingGems.size(); i < n; ++i)
	{
		SwappingGem* gem = &m_swappingGems[i];
		if (gem->m_bMoving)
		{
			//@TODO this might look nicer with some acceleration or with a little inertia
			gem->m_pos += (gem->m_bPositiveDir ? 1.f : -1.f) * gem->m_speed * dt_s;
			if ((gem->m_bPositiveDir && gem->m_pos >= gem->m_finalPos) ||
				!gem->m_bPositiveDir && gem->m_pos <= gem->m_finalPos)
			{
				gem->m_bMoving = false;
				assert(isCellSwapping(gem->m_destRow, gem->m_destCol));
				mat(gem->m_destRow, gem->m_destCol).color = gem->m_color;
				bool hasChained = solveBoardAtPos(gem->m_destRow, gem->m_destCol);
				if (!hasChained)
				{
					//solve Falling 
					//If it has chained, solveBoardAtPos already took care of the falling

					bool shouldSolveFall = false;

					//this gem has returned and didn't chain
					shouldSolveFall = !gem->hasValidPair();


					if(gem->hasValidPair())
					{
						SwappingGemsPair& pair = m_swappingGemPairs[gem->m_swapPairIdx];
						if (pair.hasFinished() && !pair.isReturning() && pair.hasChained())
						{
							shouldSolveFall = true;
						}
					}
					if (shouldSolveFall)
					{
						solveFallAtPos(gem->m_destRow, gem->m_destCol);
					}
				}

				if (gem->hasValidPair())
				{
					SwappingGemsPair& pair = m_swappingGemPairs[gem->m_swapPairIdx];

					if (pair.hasFinished() && !pair.isReturning())
					{	
						if (!hasChained && !pair.hasChained())
						{
							assert (pair.getGem1Idx() == i || pair.getGem2Idx() == i);
							int otherGemIdx = pair.getGem1Idx() + pair.getGem2Idx() - i;
							pair.setIsReturning();
							releasePair(gem->m_swapPairIdx);
							swapGems(gem->m_destRow, gem->m_destCol, m_swappingGems[otherGemIdx].m_destRow, m_swappingGems[otherGemIdx].m_destCol, false);

							//the gem pointer is invalidated from here on since 
							//swapGems pushes back into the vector and may reallocate its memory
							gem = &m_swappingGems[i];
						}
					}
					else
					{
						pair.setHasChained(hasChained);
					}
				}
			}
		}	
	}
	for (size_t i = 0; i < m_swappingGems.size(); ++i)
	{
		if (m_swappingGems[i].m_bMoving == false)
		{
			if (m_swappingGems[i].hasValidPair())
			{
				releasePair( m_swappingGems[i].m_swapPairIdx);
			}

			removeSwappingGem(i);
			// decrease i to make sure we don't skip the 
			// element placed on top of the removed one
			--i;	
		}
	}
}

void Board::updateFallingGems(float dt_s)
{
	for (size_t fallCol = 0; fallCol < kBoardCols; ++fallCol)
	{
		int endIdx = m_fallingGemsEndIdx[fallCol];
		if (endIdx < m_fallingGemsStartIdx[fallCol])
		{
			endIdx += kBoardRowsPlusOne;
		}

		for (int i = m_fallingGemsStartIdx[fallCol], n = endIdx; i < n; ++i)
		{
			int idx = i % kBoardRowsPlusOne;
			FallingGem& gem = m_fallingGems[fallCol][idx];
			//if (gem.m_bMoving)
			{
				static float g_acceleration = 9.81f;
				gem.m_speed +=  g_acceleration * kPixelsPerMeters * dt_s;
				gem.m_posY += gem.m_speed * dt_s;
				Point nextCellPoint = getCellByPos(gem.x(), gem.y() + m_tileSizeH);
				int nextRow = nextCellPoint.x;
				int col = nextCellPoint.y;
				if (nextRow >= 0)
				{
					if (nextRow >= kBoardRows || mat(nextRow, col).color != kEmptyCellColor)
					{
						int lastEmptyRow = nextRow - 1;
						//gem.m_bMoving = false;
					
						// they should stop in FirstInFirstOut order
						// @TODO: make sure they always do
						//assert(idx == m_fallingGemsStartIdx[fallCol] % kBoardRowsPlusOne);
			
						m_fallingGemsStartIdx[fallCol] = (m_fallingGemsStartIdx[fallCol] + 1) % kBoardRowsPlusOne;
						
						//@TODO: failsafe: make sure this never happens
						if (lastEmptyRow >= 0)
						{
							mat(lastEmptyRow, col).color = gem.m_color;
						}

						if (lastEmptyRow == 0 && m_bPlayerHasMoved)
						{
							//m_bPlayerHasMoved is used to make sure we don't try to solve anything
							//after the initial falling gems since everything is already solved
							
							//when no more gems are falling, solve all the column
							//@TODO: This can be optimized.
							for(int checkedRow = kBoardRows - 1; checkedRow >= 0; --checkedRow)
							{
								if (isStaticGem(mat(checkedRow, col)))
								{
									solveBoardAtPos(checkedRow, col);
								}
							}
						}
					}
				}
			}
		}
	}
}

void Board::addFallingGem(int col, int startY, int8_t color)
{
	m_fallingGems[col][m_fallingGemsEndIdx[col]].init(getTileCenterX(col), startY, color);
	m_fallingGemsEndIdx[col] = (m_fallingGemsEndIdx[col] + 1) % kBoardRowsPlusOne;
}

void Board::update(float dt_ms)
{
	//(this is a fix for a corner-case where you erase some gems while others are still falling on the same column)
	//@TODO: fix this in a nicer way 
	for (size_t col = 0; col < kBoardCols; ++col)
	{
		int endIdx = m_fallingGemsEndIdx[col];
		if (endIdx < m_fallingGemsStartIdx[col])
		{
			endIdx += kBoardRowsPlusOne;
		}
		int totalFalling = endIdx - m_fallingGemsStartIdx[col];
		if (totalFalling == 0)
		{
			for(int row = kBoardRows - 1; row >= 0; --row)
			{
				if (mat(row, col).color == kEmptyCellColor)
				{
					int8_t color = rand() % m_numGemTypes;
					Point pos = getTileCenter(row - kBoardRows, col);
					addFallingGem(col, pos.y, color);	
				}
			}
		}
	}

	if (m_bGameRunning)
	{
		m_time_ms += static_cast<int>(dt_ms);
	}
	float dt_s = dt_ms * 0.001f;

	updateSwappingGems(dt_s);
	updateFallingGems(dt_s);

	solveSelectionValidity();
}


void Board::render(SDL_Renderer* renderer)
{
	if (m_bGameRunning && m_boardState == EBS_SECOND_SELECTION)
	{
		SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
		SDL_Rect selectedRect;
		selectedRect.x = m_boardBoundsXMin + m_lastClickedCol * m_tileSizeW;
		selectedRect.y = m_boardBoundsYMin + m_lastClickedRow * m_tileSizeH;
		selectedRect.w = m_tileSizeW;
		selectedRect.h = m_tileSizeH;
		SDL_RenderDrawRect(renderer, &selectedRect);
	}

	auto& gemTextures = m_pAssetMgr->getGemTextures();
	for (int row = 0; row < kBoardRows; ++row)
	{
		for (int col = 0; col < kBoardCols; ++col)
		{
			Cell& crtCell = mat(row, col);
			if (isStaticGem(crtCell))
			{
				Point pos = getTileCenter(row, col);
				m_pGfxMgr->renderTexture(gemTextures[crtCell.color], pos.x, pos.y);
			}
		}
	}

	for (SwappingGem& gem : m_swappingGems)
	{
		if (gem.m_bMoving)
		{
			m_pGfxMgr->renderTexture(gemTextures[gem.m_color], gem.x(), gem.y());
		}
	}


	for (size_t fallCol = 0; fallCol < kBoardCols; ++fallCol)
	{
		int endIdx = m_fallingGemsEndIdx[fallCol];
		if (endIdx < m_fallingGemsStartIdx[fallCol])
		{
			endIdx += kBoardRowsPlusOne;
		}

		for (int i = m_fallingGemsStartIdx[fallCol], n = endIdx; i < n; ++i)
		{
			int idx = i % kBoardRowsPlusOne;
			FallingGem& gem = m_fallingGems[fallCol][idx];
			m_pGfxMgr->renderTexture(gemTextures[gem.m_color], gem.x(), gem.y());
		}
	}
	//for (auto& gems : m_fallingGems)
	//{
	//	for (FallingGem& gem : gems)
	//	{
	//		if (gem.m_bMoving)
	//		{
	//			m_pGfxMgr->renderTexture(gemTextures[gem.m_color], gem.x(), gem.y());
	//		}
	//	}
	//}

	DrawGrid(renderer);
}

void Board::DrawGrid(SDL_Renderer* renderer)
{
	if (m_pGfxMgr->getDebugDraw())
	{
		SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
		for (int row = 0; row <= kBoardRows; ++row)
		{
			int lineY = m_boardBoundsYMin + row * m_tileSizeH;
			SDL_RenderDrawLine(renderer, m_boardBoundsXMin, lineY, m_boardBoundsXMax, lineY);
		}

		for (int col = 0; col <= kBoardCols; ++col)
		{
			int lineX = m_boardBoundsXMin + col * m_tileSizeW;
			SDL_RenderDrawLine(renderer, lineX, m_boardBoundsYMin, lineX, m_boardBoundsYMax);
		}
	}
}

int Board::getTileCenterX(int col) const 
{ 
	return m_boardBoundsXMin + m_paddingW / 2 + col * m_tileSizeW;
}
int Board::getTileCenterY(int row) const 
{
	return m_boardBoundsYMin + m_paddingH / 2 + row * m_tileSizeH;
}
Point Board::getTileCenter(int row, int col) const 
{
	return Point(getTileCenterX(col), getTileCenterY(row));
}

int Board::getRowByPos(int y) const
{
	return (y - m_boardBoundsYMin - m_paddingH / 2) / m_tileSizeH;
}
int Board::getColByPos(int x) const
{
	return (x - m_boardBoundsXMin - m_paddingW / 2) / m_tileSizeW;
}

Utils::Point Board::getCellByPos(int x, int y) const
{
	return Point(getRowByPos(y), getColByPos(x));
}

Board::SwappingGem::SwappingGem(int startX, int startY, int destX, int destY, int swapPairIdx, int dest_row, int dest_col, int8_t color)
{
	assert(color != kEmptyCellColor && color != kSwapCellColor);
	m_swapPairIdx = swapPairIdx;

	m_destRow = dest_row;
	m_destCol = dest_col;

	m_bMoving = true;
	m_bAxisX = (startY == destY);
	assert((startX == destX) != m_bAxisX);

	m_bPositiveDir = ((destX - startX) + (destY - startY)) > 0;

	m_speed = 4.f * kPixelsPerMeters;

	if (m_bAxisX)
	{
		m_pos = static_cast<float>(startX);
		m_finalPos = destX;
		m_otherAxisPos = startY;
	}
	else
	{
		m_pos = static_cast<float>(startY);
		m_finalPos = destY;
		m_otherAxisPos = startX;
	}
	m_color = color;
}
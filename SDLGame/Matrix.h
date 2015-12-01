#ifndef MATRIX_IMPL_H
#define MATRIX_IMPL_H
#include <assert.h>

template <class T, size_t ROWS, size_t COLS>
class Matrix
{
protected:
	T matrix[ROWS][COLS];
public:
	int GetNumRows() const { return ROWS; }
	int GetNumCols() const { return COLS; }

	T& operator() (size_t row, size_t col)
	{
		assert(row < ROWS && col < COLS);
		return matrix[row][col];
	}
	T operator() (size_t row, size_t col) const
	{
		assert(row < ROWS && col < COLS);
		return matrix[row][col];
	}
};
#endif//MATRIX_IMPL_H

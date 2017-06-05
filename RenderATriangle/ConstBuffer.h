#include <d3d11.h>
#include <D3DX11.h>
#include <D3Dcompiler.h>
#include <xnamath.h>

#include <vector>
#include <string>
#include <assert.h>
using namespace std;

class VariableInfo
{
public:
	int		size;
	int		offset;
	int     slot;
	string	name;
};

class ConstantBufferInfo
{
public:
	ConstantBufferInfo(int _size) : slot(0), numOfVariables(0)
	{
		assert(_size >= 4);
		size = _size;
		mSystemMemoryBuffer = new char[_size];
	}
	~ConstantBufferInfo()
	{
		delete [] mSystemMemoryBuffer;
	}
	bool setBool(int offset, bool b)
	{
		if (offset < 0 || offset > size - sizeof(float))
			return false;
		CopyMemory((char*)mSystemMemoryBuffer + offset, &b, sizeof(float));
		return true;
	}
	bool setBoolArray(int offset, const bool* b, int count)
	{
		if (offset < 0 || offset > size - count * 4 * sizeof(float))
			return false;
		for (int i = 0; i < count; i++)
		{
			CopyMemory((char*)mSystemMemoryBuffer + offset + i * 4 * sizeof(float), &b[i], sizeof(float));
		}
		return true;
	}
	bool setFloat(int offset, float f)
	{
		if (offset < 0 || offset > size - sizeof(float))
			return false;
		CopyMemory((char*)mSystemMemoryBuffer + offset, &f, sizeof(float));
		return true;
	}
	bool setFloatArray(int offset, const float* f, int count)
	{
		if (offset < 0 || offset > size - count * 4 * sizeof(float))
			return false;
		for (int i = 0; i < count; i++)
		{
			CopyMemory((char*)mSystemMemoryBuffer + offset + i * 4 * sizeof(float), &f[i], sizeof(float));
		}
		return true;
	}
	bool setInt(int offset, int i)
	{
		if (offset < 0 || offset > size - sizeof(float))
			return false;
		CopyMemory((char*)mSystemMemoryBuffer + offset, &i, sizeof(float));
		return true;
	}
	bool setIntArray(int offset, const int* array, int count)
	{
		if (offset < 0 || offset > size - count * 4 * sizeof(float))
			return false;
		for (int i = 0; i < count; i++)
		{
			CopyMemory((char*)mSystemMemoryBuffer + offset + i * 4 * sizeof(float), &array[i], sizeof(float));
		}
		return true;
	}
	bool setMatrix(int offset, const XMMATRIX& matrix)
	{
		if (offset < 0 || offset > size - sizeof(matrix))
			return false;
		CopyMemory((char*)mSystemMemoryBuffer + offset, &matrix, sizeof(XMMATRIX));
		return true;
	}
	bool setMatrixArray(int offset, const XMMATRIX* matrix, int count)
	{
		if (offset < 0 || offset > size - count * sizeof(XMMATRIX))
			return false;
		CopyMemory((char*)mSystemMemoryBuffer + offset, matrix, count * sizeof(XMMATRIX));
		return true;
	}
	bool setVector(int offset, const XMFLOAT2& v2)
	{
		if (offset < 0 || offset > size -  sizeof(XMFLOAT4))
			return false;
		CopyMemory((char*)mSystemMemoryBuffer + offset, &v2, sizeof(XMFLOAT2));
		return true;
	}
	bool setVector(int offset, const XMFLOAT3& v3)
	{
		if (offset < 0 || offset > size -  sizeof(XMFLOAT4))
			return false;
		CopyMemory((char*)mSystemMemoryBuffer + offset, &v3, sizeof(XMFLOAT3));
		return true;
	}
	bool setVector(int offset, const XMFLOAT4& v4)
	{
		if (offset < 0 || offset > size -  sizeof(XMFLOAT4))
			return false;
		CopyMemory((char*)mSystemMemoryBuffer + offset, &v4, sizeof(XMFLOAT4));
		return true;
	}
	bool setVectorArray(int offset, const XMFLOAT2* v2s, int count)
	{
		if (offset < 0 || offset > size - count * sizeof(XMFLOAT4))
			return false;
		for (int i = 0; i < count; i++)
		{
			CopyMemory((char*)mSystemMemoryBuffer + offset + i * sizeof(XMFLOAT4), &v2s[i], sizeof(XMFLOAT2));
		}
		return true;
	}
	bool setVectorArray(int offset, const XMFLOAT3* v3s, int count)
	{
		if (offset < 0 || offset > size - count * sizeof(XMFLOAT4))
			return false;
		for (int i = 0; i < count; i++)
		{
			CopyMemory((char*)mSystemMemoryBuffer + offset + i * sizeof(XMFLOAT4), &v3s[i], sizeof(XMFLOAT3));
		}
		return true;
	}
	bool setVectorArray(int offset, const XMFLOAT4* v4s, int count)
	{
		if (offset < 0 || offset > size - count * sizeof(XMFLOAT4))
			return false;
		for (int i = 0; i < count; i++)
		{
			CopyMemory((char*)mSystemMemoryBuffer + offset + i * sizeof(XMFLOAT4), &v4s[i], sizeof(XMFLOAT4));
		}
		return true;
	}
	bool setValue(int offset, const void* pData, int bytes)
	{
		if (offset < 0 || offset > size -  bytes)
			return false;
		CopyMemory((char*)mSystemMemoryBuffer + offset, pData, bytes);
		return true;
	}


public:
	char* mSystemMemoryBuffer;
public:
	int slot;
	int size;
	int numOfVariables;
	string name;
	vector<VariableInfo> variables;
};
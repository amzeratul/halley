#include "dx11_buffer.h"
#include "halley/utils/utils.h"
#include "dx11_video.h"
#include <minwinbase.h>
using namespace Halley;

DX11Buffer::DX11Buffer(DX11Video& video, Type type, size_t initialSize)
	: video(video)
	, type(type)
{
	if (initialSize > 0) {
		resize(initialSize);
	}
}

DX11Buffer::DX11Buffer(DX11Buffer&& other) noexcept
	: video(other.video)
	, type(other.type)
	, buffer(other.buffer)
	, curSize(other.curSize)
	, curPos(other.curPos)
	, lastSize(other.lastSize)
	, lastPos(other.lastPos)
	, waitingReset(other.waitingReset)
{
	other.buffer = nullptr;
}

DX11Buffer::~DX11Buffer()
{
	clear();
}

void DX11Buffer::setData(gsl::span<const gsl::byte> data)
{
	Expects(data.size_bytes() > 0);

#ifdef WINDOWS_STORE
	constexpr bool useImmutable = false;
#else
	constexpr bool useImmutable = false;
#endif

	if (useImmutable) {
		clear();

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_IMMUTABLE;
		bd.ByteWidth = UINT(data.size_bytes());
		bd.CPUAccessFlags = 0;
		switch (type) {
		case Type::Constant:
			bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			break;
		case Type::Index:
			bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			break;
		case Type::Vertex:
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			break;
		}

		D3D11_SUBRESOURCE_DATA resData;
		resData.pSysMem = data.data();
		resData.SysMemPitch = 0;
		resData.SysMemSlicePitch = 0;

		video.getDevice().CreateBuffer(&bd, &resData, &buffer);
	} else {
		if (size_t(data.size_bytes()) > curSize) {
			resize(size_t(data.size_bytes()));
		}

		lastPos = curPos;
		lastSize = alignUp(size_t(data.size_bytes()), size_t(256));

		D3D11_MAPPED_SUBRESOURCE ms;
		ZeroMemory(&ms, sizeof(ms));

		auto& devCon = video.getDeviceContext();
		HRESULT result = devCon.Map(buffer, 0, waitingReset ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE, 0, &ms);
		if (!SUCCEEDED(result)) {
			throw Exception("Failed to map buffer memory", HalleyExceptions::VideoPlugin);
		}
		void* dst = reinterpret_cast<char*>(ms.pData) + lastPos;
		memcpy(dst, data.data(), data.size_bytes());
		devCon.Unmap(buffer, 0);

		curPos = alignUp(size_t(lastPos + lastSize), size_t(256));
		waitingReset = false;
	}
}

ID3D11Buffer*& DX11Buffer::getBuffer()
{
	return buffer;
}

UINT DX11Buffer::getOffset() const
{
	return UINT(lastPos);
}

UINT DX11Buffer::getLastSize() const
{
	return UINT(lastSize);
}

bool DX11Buffer::canFit(size_t size) const
{
	return curPos + alignUp(size, size_t(256)) <= curSize;
}

void DX11Buffer::reset()
{
	waitingReset = true;
	curPos = 0;
	lastPos = 0;
}

void DX11Buffer::clear()
{
	if (buffer) {
		buffer->Release();
		buffer = nullptr;
	}
	curSize = 0;
}

void DX11Buffer::resize(size_t requestedSize)
{
	size_t targetSize = std::max(size_t(256), nextPowerOf2(requestedSize));

	if (buffer) {
		buffer->Release();
		buffer = nullptr;
	}

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));

	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = UINT(targetSize);
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	switch (type) {
	case Type::Constant:
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		break;
	case Type::Index:
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		break;
	case Type::Vertex:
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		break;
	}

	HRESULT result = video.getDevice().CreateBuffer(&bd, nullptr, &buffer);
	if (result != S_OK) {
		throw Exception("Unable to create DX buffer with size " + toString(targetSize), HalleyExceptions::VideoPlugin);
	}

	curSize = targetSize;
	reset();
}

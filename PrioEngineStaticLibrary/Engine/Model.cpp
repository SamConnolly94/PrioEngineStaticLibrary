#include "Model.h"


CModel::CModel()
{
	logger->GetInstance().MemoryAllocWriteLine(typeid(this).name());
}


CModel::~CModel()
{
	// Write an allocation message to our memory log.
	logger->GetInstance().MemoryAllocWriteLine(typeid(this).name());
}

void CModel::Shutdown()
{
}

void CModel::UpdateMatrices()
{
	// Rotation
	D3DXMATRIX translation;
	D3DXMATRIX scale;
	D3DXMATRIX matrixRotationX;
	D3DXMATRIX matrixRotationY;
	D3DXMATRIX matrixRotationZ;

	// Calculate the rotation of the model.
	float rotX = (GetRotationX() * PrioEngine::kPi) / 180.0f;
	float rotY = (GetRotationY() * PrioEngine::kPi) / 180.0f;
	float rotZ = (GetRotationZ() * PrioEngine::kPi) / 180.0f;

	D3DXMatrixRotationX(&matrixRotationX, rotX);
	D3DXMatrixRotationY(&matrixRotationY, rotY);
	D3DXMatrixRotationZ(&matrixRotationZ, rotZ);

	// Calculate scaling.
	D3DXMatrixScaling(&scale, GetScaleX(), GetScaleY(), GetScaleZ());

	// Calculate the translation of the model.
	D3DXMatrixTranslation(&translation, GetPosX(), GetPosY(), GetPosZ());

	// Calculate the world matrix
	mWorldMatrix = scale * matrixRotationX * matrixRotationY * matrixRotationZ * translation;
}

void CModel::RenderBuffers(ID3D11DeviceContext* deviceContext, int subMeshIndex, ID3D11Buffer* &vertexBuffer, ID3D11Buffer* &indexBuffer, unsigned int stride)
{
	unsigned int offset;

	offset = 0;

	// Set the vertex buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

	// Set the index buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
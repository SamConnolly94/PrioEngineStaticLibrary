#include "ModelControl.h"



CModelControl::CModelControl()
{
	mpParent = nullptr;

	mPosition.x = 0.0f;
	mPosition.y = 0.0f;
	mPosition.z = 0.0f;

	mRotation.x = 0.0f;
	mRotation.y = 0.0f;
	mRotation.z = 0.0f;

	mScale.x = 1.0f;
	mScale.y = 1.0f;
	mScale.z = 1.0f;
}


CModelControl::~CModelControl()
{
}

float CModelControl::ToRadians(float degrees)
{
	const float kPi = 3.14159265359f;
	return degrees * (kPi / 180.0f);
}

void CModelControl::RotateX(float x)
{
	mRotation.x += x;
}

void CModelControl::RotateY(float y)
{
	mRotation.y += y;
}

void CModelControl::RotateZ(float z)
{
	mRotation.z += z;
}

float CModelControl::GetRotationX()
{
	return mRotation.x;
}

float CModelControl::GetRotationY()
{
	return mRotation.y;
}

float CModelControl::GetRotationZ()
{
	return mRotation.z;
}

D3DXVECTOR3 CModelControl::GetRotation()
{
	return mRotation;
}

void CModelControl::SetRotationX(float x)
{
	mRotation.x = x;
}

void CModelControl::SetRotationY(float y)
{
	mRotation.y = y;
}

void CModelControl::SetRotationZ(float z)
{
	mRotation.z = z;
}

void CModelControl::SetRotation(float x, float y, float z)
{
	mRotation.x = x;
	mRotation.y = y;
	mRotation.z = z;
}

void CModelControl::MoveX(float x)
{
	mPosition.x += x;
}

void CModelControl::MoveY(float y)
{
	mPosition.y += y;
}

void CModelControl::MoveZ(float z)
{
	mPosition.z += z;
}

float CModelControl::GetPosX()
{
	if (mpParent != nullptr)
	{
		return (mPosition.x + mpParent->GetPosX());
	}
	return mPosition.x;
}

float CModelControl::GetPosY()
{
	if (mpParent != nullptr)
	{
		return (mPosition.y + mpParent->GetPosY());
	}
	return mPosition.y;
}

float CModelControl::GetPosZ()
{
	if (mpParent != nullptr)
	{
		return (mPosition.z + mpParent->GetPosZ());
	}
	return mPosition.z;
}

D3DXVECTOR3 CModelControl::GetPos()
{
	if (mpParent != nullptr)
	{
		return (mPosition + mpParent->GetPos());
	}
	return mPosition;
}

void CModelControl::SetXPos(float x)
{
	mPosition.x = x;
}

void CModelControl::SetYPos(float y)
{
	mPosition.y = y;
}

void CModelControl::SetZPos(float z)
{
	mPosition.z = z;
}

void CModelControl::SetPos(float x, float y, float z)
{
	mPosition.x = x;
	mPosition.y = y;
	mPosition.z = z;
}

void CModelControl::ScaleX(float x)
{
	mScale.x += x;
}

void CModelControl::ScaleY(float y)
{
	mScale.y += y;
}

void CModelControl::ScaleZ(float z)
{
	mScale.z += z;
}

void CModelControl::Scale(float value)
{
	mScale.x += value;
	mScale.y += value;
	mScale.z += value;
}

float CModelControl::GetScaleX()
{
	return mScale.x;
}

float CModelControl::GetScaleY()
{
	return mScale.y;
}

float CModelControl::GetScaleZ()
{
	return mScale.z;
}

D3DXVECTOR3 CModelControl::GetScale()
{
	return mScale;
}

float CModelControl::GetScaleRadius(float initialRadius)
{
	float radius = 0.0f;

	radius += mScale.x * initialRadius;
	radius += mScale.y * initialRadius;
	radius += mScale.z * initialRadius;

	return radius;
}

void CModelControl::SetScaleX(float x)
{
	mScale.x = x;
}

void CModelControl::SetScaleY(float y)
{
	mScale.y = y;
}

void CModelControl::SetScaleZ(float z)
{
	mScale.z = z;
}

void CModelControl::SetScale(float x, float y, float z)
{
	mScale.x = x;
	mScale.y = y;
	mScale.z = z;
}

void CModelControl::SetScale(float value)
{
	mScale.x = value;
	mScale.y = value;
	mScale.z = value;
}

void CModelControl::AttatchToParent(CModelControl * parent)
{
	mpParent = parent;
}

void CModelControl::SeperateFromParent()
{
	mpParent = nullptr;
}

void CModelControl::UpdateMatrices()
{
		// Rotation
		D3DXMATRIX matrixRotationX;
		D3DXMATRIX matrixRotationY;
		D3DXMATRIX matrixRotationZ;
		// Position
		D3DXMATRIX matrixTranslation;

		// Calculate the rotation of the camera.
		D3DXMatrixRotationX(&matrixRotationX, ToRadians(mRotation.x));
		D3DXMatrixRotationY(&matrixRotationY, ToRadians(mRotation.y));
		D3DXMatrixRotationZ(&matrixRotationZ, ToRadians(mRotation.z));

		// Calculate the translation of the camera.
		D3DXMatrixTranslation(&matrixTranslation, mPosition.x, mPosition.y, mPosition.z);

		// Calculate the world matrix
		mWorldMatrix = matrixRotationZ * matrixRotationX * matrixRotationY * matrixTranslation;
}

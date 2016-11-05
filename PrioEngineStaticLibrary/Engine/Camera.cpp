#include "Camera.h"



CCamera::CCamera(int screenWidth, int screenHeight, float fov, float nearClip, float farClip)
{
	mScreenWidth = screenWidth;
	mScreenHeight = screenHeight;

	mFov = fov;
	mFarClip = farClip;
	mNearClip = nearClip;

	// Initialise vector 3's
	mPosition = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	mRotation = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

	UpdateMatrices();
}

CCamera::~CCamera()
{
}

void CCamera::SetPosition(float x, float y, float z)
{
	mPosition.x = x;
	mPosition.y = y;
	mPosition.z = z;
}

void CCamera::SetPositionX(float x)
{
	mPosition.x = x;
}

void CCamera::SetPosizionY(float y)
{
	mPosition.y = y;
}

void CCamera::SetPositionZ(float z)
{
	mPosition.z = z;
}

void CCamera::MoveX(float x)
{
	mPosition.x += x;
}

void CCamera::MoveY(float y)
{
	mPosition.y += y;
}

void CCamera::MoveZ(float z)
{
	mPosition.z += z;
}

void CCamera::MoveLocalX(float speed)
{
	mPosition.x += mWorldMatrix._11 * speed;
	mPosition.y += mWorldMatrix._12 * speed;
	mPosition.z += mWorldMatrix._13 * speed;
}

void CCamera::MoveLocalY(float speed)
{
	mPosition.x += mWorldMatrix._21 * speed;
	mPosition.y += mWorldMatrix._22 * speed;
	mPosition.z += mWorldMatrix._23 * speed;
}

void CCamera::MoveLocalZ(float speed)
{
	mPosition.x += mWorldMatrix._31 * speed;
	mPosition.y += mWorldMatrix._32 * speed;
	mPosition.z += mWorldMatrix._33 * speed;
}

void CCamera::RotateX(float x)
{
	mRotation.x += x;
}

void CCamera::RotateY(float y)
{
	mRotation.y += y;
}

void CCamera::RotateZ(float z)
{
	mRotation.z += z;
}

float CCamera::GetRotX()
{
	return mRotation.x;
}

float CCamera::GetRotY()
{
	return mRotation.y;
}

float CCamera::GetRotZ()
{
	return mRotation.z;
}

float CCamera::GetX()
{
	return mPosition.x;
}

float CCamera::GetY()
{
	return mPosition.y;
}

float CCamera::GetZ()
{
	return mPosition.z;
}

void CCamera::SetRotation(float x, float y, float z)
{
	mRotation.x = x;
	mRotation.y = y;
	mRotation.z = z;
}

D3DXVECTOR3 CCamera::GetPosition()
{
	return mPosition;
}

D3DXVECTOR3 CCamera::GetRotation()
{
	return mRotation;
}

void CCamera::Render()
{
	UpdateMatrices();
}

void CCamera::GetViewMatrix(D3DXMATRIX & viewMatrix)
{
	viewMatrix = mViewMatrix;
}

/* Updates the elements of matrices used before rendering. 
* Credit to Laurent Noel for this class.
*/
void CCamera::UpdateMatrices()
{
	// Rotation
	D3DXMATRIX matrixRotationX;
	D3DXMATRIX matrixRotationY;
	D3DXMATRIX matrixRotationZ;
	// Position
	D3DXMATRIX matrixTranslation;

	// Calculate the rotation of the camera.
	D3DXMatrixRotationX(&matrixRotationX, mRotation.x);
	D3DXMatrixRotationY(&matrixRotationY, mRotation.y);
	D3DXMatrixRotationZ(&matrixRotationZ, mRotation.z);
	
	// Calculate the translation of the camera.
	D3DXMatrixTranslation(&matrixTranslation, mPosition.x, mPosition.y, mPosition.z);

	// Calculate the world matrix
	mWorldMatrix = matrixRotationZ * matrixRotationX * matrixRotationY * matrixTranslation;

	// The rendering pipeline requires the inverse of the camera world matrix, so calculate that (Thanks Laurent!)
	D3DXMatrixInverse(&mViewMatrix, NULL, &mWorldMatrix);

	// Initialise the projection matrix.
	float aspectRatio = static_cast<float>(mScreenWidth / mScreenHeight);
	D3DXMatrixPerspectiveFovLH(&mProjMatrix, mFov, aspectRatio, mNearClip, mFarClip);

	// Combine the view and proj matrix into one matrix. This comes in useful for efficiency, as it gets done in the vertex shader every time it has to draw a vertex shader.
	mViewProjMatrix = mViewMatrix * mProjMatrix;
}

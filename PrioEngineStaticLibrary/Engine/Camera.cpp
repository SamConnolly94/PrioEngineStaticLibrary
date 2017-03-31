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

void CCamera::SetPosition(D3DXVECTOR3 pos)
{
	mPosition.x = pos.x;
	mPosition.y = pos.y;
	mPosition.z = pos.z;
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

void CCamera::GetWorldMatrix(D3DXMATRIX & worldMatrix)
{
	worldMatrix = mWorldMatrix;
}

void CCamera::GetViewMatrix(D3DXMATRIX & viewMatrix)
{
	viewMatrix = mViewMatrix;
}

void CCamera::GetReflectionView(D3DXMATRIX& view)
{
	D3DXMATRIX invertYMatrix = D3DXMATRIX(	1.0F, 0.0F, 0.0F, 0.0F, 
											0.0F, -1.0F, 0.0F, 0.0F, 
											0.0F, 0.0F, 1.0F, 0.0F, 
											0.0F, 0.0F, 0.0F, 1.0F);
	view = mViewMatrix * invertYMatrix;
}

void CCamera::GetViewProjMatrix(D3DXMATRIX & ViewProjMatrix, D3DXMATRIX proj)
{
	ViewProjMatrix = mViewMatrix * proj;
	mViewProjMatrix = ViewProjMatrix;
}

void CCamera::RenderReflection(float waterHeight)
{
	D3DXVECTOR3 up;

	up.x = 0.0f;
	up.y = 1.0f;
	up.z = 0.0f;

	D3DXVECTOR3 position;

	position.x = mPosition.x;
	position.y = -mPosition.y + (waterHeight * 2.0f);
	position.z = mPosition.z;

	D3DXVECTOR3 lookAt;
	lookAt.x = 0.0f;
	lookAt.y = 0.0f;
	lookAt.z = 1.0f;

	float yaw = ToRadians(-mRotation.x);
	float pitch = ToRadians(mRotation.y);
	float roll = ToRadians(mRotation.z);

	D3DXMATRIX rotationMatrix;
	D3DXMatrixRotationYawPitchRoll(&rotationMatrix, yaw, pitch, roll);

	// Rotate the view at the origin
	D3DXVec3TransformCoord(&lookAt, &lookAt, &rotationMatrix);
	D3DXVec3TransformCoord(&up, &up, &rotationMatrix);

	lookAt = position + lookAt;

	D3DXMatrixLookAtLH(&mReflectionMatrix, &position, &lookAt, &up);
}

void CCamera::GetReflectionViewMatrix(D3DXMATRIX & reflectionView)
{
	reflectionView = mReflectionMatrix;
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
	D3DXMatrixRotationX(&matrixRotationX, ToRadians(mRotation.x));
	D3DXMatrixRotationY(&matrixRotationY, ToRadians(mRotation.y));
	D3DXMatrixRotationZ(&matrixRotationZ, ToRadians(mRotation.z));
	
	// Calculate the translation of the camera.
	D3DXMatrixTranslation(&matrixTranslation, mPosition.x, mPosition.y, mPosition.z);

	// Calculate the world matrix
	mWorldMatrix = matrixRotationZ * matrixRotationX * matrixRotationY * matrixTranslation;

	// The rendering pipeline requires the inverse of the camera world matrix, so calculate that (Thanks Laurent!)
	D3DXMatrixInverse(&mViewMatrix, NULL, &mWorldMatrix);

	//// Initialise the projection matrix.
	//float aspectRatio = static_cast<float>(mScreenWidth / mScreenHeight);
	//D3DXMatrixPerspectiveFovLH(&mProjMatrix, mFov, aspectRatio, mNearClip, mFarClip);

	//// Combine the view and proj matrix into one matrix. This comes in useful for efficiency, as it gets done in the vertex shader every time it has to draw a vertex shader.
	//mViewProjMatrix = mViewMatrix * mProjMatrix;
}

float CCamera::ToRadians(float degrees)
{
	const float kPi = 3.14159265359f;

	return degrees * (kPi / 180.0f);

}

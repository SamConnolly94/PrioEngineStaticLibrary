#include "Light.h"



CLight::CLight()
{
	mDiffuseColour = D3DXVECTOR4{ 0.0f, 0.0f, 0.0f, 0.0f };
	mAmbientColour = D3DXVECTOR4{ 0.0f, 0.0f, 0.0f, 0.0f };
}

CLight::~CLight()
{
}

void CLight::SetAmbientColour(D3DXVECTOR4 colour)
{
	mAmbientColour	 = D3DXVECTOR4(colour.x, colour.y, colour.z, colour.w);
}

void CLight::SetDiffuseColour(D3DXVECTOR4 colour)
{
	mDiffuseColour = D3DXVECTOR4(colour.x, colour.y, colour.z, colour.w);
}

void CLight::SetDirection(D3DXVECTOR3 direction)
{
	mDirection = D3DXVECTOR3(direction.x, direction.y, direction.z);
}

void CLight::SetSpecularColour(D3DXVECTOR4 colour)
{
	mSpecularColour = colour;
}

void CLight::SetSpecularPower(float power)
{
	mSpecularPower = power;
}

D3DXVECTOR4 CLight::GetDiffuseColour()
{
	return mDiffuseColour;
}

D3DXVECTOR4 CLight::GetAmbientColour()
{
	return mAmbientColour;
}

D3DXVECTOR3 CLight::GetDirection()
{
	return mDirection;
}

D3DXVECTOR4 CLight::GetSpecularColour()
{
	return mSpecularColour;
}

float CLight::GetSpecularPower()
{
	return mSpecularPower;
}

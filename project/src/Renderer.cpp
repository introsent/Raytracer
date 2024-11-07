//External includes
#include "SDL.h"
#include "SDL_surface.h"
#include <execution>

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Matrix.h"
#include "Material.h"
#include "Scene.h"
#include "Utils.h"

#define PARALLEL_EXECUTION

using namespace dae;

Renderer::Renderer(SDL_Window * pWindow) :
	m_pWindow(pWindow),
	m_pBuffer(SDL_GetWindowSurface(pWindow))
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	m_pBufferPixels = static_cast<uint32_t*>(m_pBuffer->pixels);
}

void Renderer::Render(Scene* pScene) const
{
	Camera& camera = pScene->GetCamera();
	const Matrix cameraToWorld = camera.CalculateCameraToWorld();

	float aspectRatio = float(m_Width) / m_Height;

    float FOV = tan(camera.fovAngle * (PI / 180.f) / 2.f);

	//uint32_t amountOfPixels{ uint32_t(m_Width * m_Height) };

	//for (uint32_t pixelIndex{}; pixelIndex < amountOfPixels; ++pixelIndex)
	//{
	//	RenderPixel(pScene, pixelIndex, FOV, aspectRatio, cameraToWorld, camera.origin);
	//}

#if defined(PARALLEL_EXECUTION)
	uint32_t amountOfPixels{ uint32_t(m_Width * m_Height) };
	std::vector<uint32_t> pixelIndices{};

	pixelIndices.reserve(amountOfPixels);
	for (uint32_t index{}; index < amountOfPixels; ++index) pixelIndices.emplace_back(index);

	std::for_each(std::execution::par, pixelIndices.begin(), pixelIndices.end(), [&](int i) {
		RenderPixel(pScene, i, FOV, aspectRatio, cameraToWorld, camera.origin);
	});
#else 
	uint32_t amountOfPixels{ uint32_t(m_Width * m_Height) };

	for (uint32_t pixelIndex{}; pixelIndex < amountOfPixels; ++pixelIndex)
	{
		RenderPixel(pScene, pixelIndex, FOV, aspectRatio, cameraToWorld, camera.origin);
	}
#endif

    SDL_UpdateWindowSurface(m_pWindow);

}

void Renderer::RenderPixel(Scene* pScene, uint32_t pixelIndex, float FOV, float aspectRatio, const Matrix cameraToWorld, const Vector3 cameraOrigin) const
{
    auto materials{ pScene->GetMaterials() };
    const uint32_t px{ pixelIndex % m_Width }, py{ pixelIndex / m_Width };

    float rx{ px + 0.5f }, ry{ py + 0.5f };
    float  cx{ (2 * (rx / float(m_Width)) - 1) * aspectRatio * FOV };
    float cy{ (1 - (2 * (ry / float(m_Height)))) * FOV };


	Vector3 rayDirection{ (2 * (px + 0.5f) / m_Width - 1) * aspectRatio * FOV ,   (1 - 2 * (py + 0.5f) / m_Height) * FOV, 1.f };

	rayDirection.Normalize();

	Ray viewRay{ cameraOrigin, cameraToWorld.TransformVector(rayDirection) };

	ColorRGB finalColor{  };
	HitRecord closestHit{ };

	pScene->GetClosestHit(viewRay, closestHit);;

	if (closestHit.didHit)
	{

		//finalColor = materials[closestHit.materialIndex]->Shade();
		for (const Light& lightPtr : pScene->GetLights())
		{
			float distanceFromHitToLight;

			Vector3 lightDirection{ LightUtils::GetDirectionToLight(lightPtr, closestHit.origin) };
			distanceFromHitToLight = lightDirection.Magnitude();
			lightDirection.Normalize();

			if (m_ShadowsEnabled)
			{
				if (pScene->DoesHit(Ray(closestHit.origin, lightDirection, 0.0001f, distanceFromHitToLight))) continue;
			}

			float cosOfAngle{ Vector3::Dot(closestHit.normal, lightDirection) };
			if (cosOfAngle < 0) continue;


			//finalColor = ColorRGB(cosOfAngle, cosOfAngle, cosOfAngle);
			Vector3 hitToCameraDirection = (closestHit.origin, cameraOrigin).Normalized();

			ColorRGB brdf{ materials[closestHit.materialIndex]->Shade(closestHit, lightDirection, hitToCameraDirection) };

			if (m_CurrentLightingMode == LightingMode::ObservedArea)
			{
				finalColor += ColorRGB(cosOfAngle, cosOfAngle, cosOfAngle);
			}

			if (m_CurrentLightingMode == LightingMode::Radiance)
			{
				finalColor += LightUtils::GetRadiance(lightPtr, closestHit.origin);
			}

			if (m_CurrentLightingMode == LightingMode::BRDF)
			{
				finalColor += brdf;
			}

			if (m_CurrentLightingMode == LightingMode::Combined)
			{
				finalColor += cosOfAngle * LightUtils::GetRadiance(lightPtr, closestHit.origin) * brdf;
			}
		}
		finalColor.MaxToOne();

		m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBuffer->format,
			static_cast<uint8_t>(finalColor.r * 255),
			static_cast<uint8_t>(finalColor.g * 255),
			static_cast<uint8_t>(finalColor.b * 255));

    }

}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBuffer, "RayTracing_Buffer.bmp");
}

void Renderer::CycleLightingMode()
{
	switch (m_CurrentLightingMode)
	{
	case LightingMode::Combined:
		m_CurrentLightingMode = LightingMode::ObservedArea;
		break;
	case LightingMode::ObservedArea:
		m_CurrentLightingMode = LightingMode::Radiance;
		break;
	case LightingMode::Radiance:
		m_CurrentLightingMode = LightingMode::BRDF;
		break;
	case LightingMode::BRDF:
		m_CurrentLightingMode = LightingMode::Combined;
		break;
	}
}

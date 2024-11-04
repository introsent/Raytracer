//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Matrix.h"
#include "Material.h"
#include "Scene.h"
#include "Utils.h"

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
	auto& materials = pScene->GetMaterials();
	auto& lights = pScene->GetLights();


	float aspectRatio = float(m_Width) / m_Height;

    float offsets[4][2] = {
                {0.25f, 0.25f}, {0.75f, 0.25f},
                {0.25f, 0.75f}, {0.75f, 0.75f}
    };

    int sampleCount = 4; // 4x supersampling (2x2 grid)
    float FOV = tan(camera.fovAngle * (PI / 180.f) / 2.f);

    for (int px = 0; px < m_Width; ++px) {
        for (int py = 0; py < m_Height; ++py) {
            ColorRGB finalColor{ 0, 0, 0 }; // Initialize final color to accumulate samples

            for (int i = 0; i < sampleCount; ++i) {
                float offsetX = offsets[i][0];
                float offsetY = offsets[i][1];

                // Adjust ray direction with the offset for anti-aliasing
                Vector3 rayDirection{
                    (2 * (px + offsetX) / m_Width - 1) * aspectRatio * FOV,
                    (1 - 2 * (py + offsetY) / m_Height) * FOV,
                    1.0f
                };
                rayDirection.Normalize();

                const Matrix cameraToWorld = camera.CalculateCameraToWorld();
                Ray viewRay{ camera.origin, cameraToWorld.TransformVector(rayDirection) };

                ColorRGB sampleColor{ 0, 0, 0 };
                HitRecord closestHit{};

                pScene->GetClosestHit(viewRay, closestHit);

                if (closestHit.didHit) {
                    for (const Light& lightPtr : lights) {
                        float distanceFromHitToLight;
                        Vector3 lightDirection = LightUtils::GetDirectionToLight(lightPtr, closestHit.origin);
                        distanceFromHitToLight = lightDirection.Magnitude();
                        lightDirection.Normalize();

                        if (m_ShadowsEnabled && pScene->DoesHit(Ray(closestHit.origin, lightDirection, 0.0001f, distanceFromHitToLight)))
                            continue;

                        float cosOfAngle = Vector3::Dot(closestHit.normal, lightDirection);
                        if (cosOfAngle < 0) continue;

                        Vector3 hitToCameraDirection = (camera.origin - closestHit.origin).Normalized();
                        ColorRGB brdf = materials[closestHit.materialIndex]->Shade(closestHit, lightDirection, hitToCameraDirection);

                        switch (m_CurrentLightingMode) {
                        case LightingMode::ObservedArea:
                            sampleColor += ColorRGB(cosOfAngle, cosOfAngle, cosOfAngle);
                            break;
                        case LightingMode::Radiance:
                            sampleColor += LightUtils::GetRadiance(lightPtr, closestHit.origin);
                            break;
                        case LightingMode::BRDF:
                            sampleColor += brdf;
                            break;
                        case LightingMode::Combined:
                            sampleColor += cosOfAngle * LightUtils::GetRadiance(lightPtr, closestHit.origin) * brdf;
                            break;
                        }
                    }
                }

                sampleColor.MaxToOne();
                finalColor += sampleColor; // Accumulate sample colors
            }

            // Average the color over the samples
            finalColor /= float(sampleCount);

            // Convert final color to SDL format and update the pixel buffer
            m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB(
                m_pBuffer->format,
                static_cast<uint8_t>(finalColor.r * 255),
                static_cast<uint8_t>(finalColor.g * 255),
                static_cast<uint8_t>(finalColor.b * 255)
            );
        }
    }

    // Update SDL Surface
    SDL_UpdateWindowSurface(m_pWindow);

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

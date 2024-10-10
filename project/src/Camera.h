#pragma once
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Maths.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle) :
			origin{ _origin },
			fovAngle{ _fovAngle }
		{
		}


		Vector3 origin{};
		float fovAngle{ 90.f };

		Vector3 forward{ Vector3::UnitZ };
		Vector3 up{ Vector3::UnitY };
		Vector3 right{ Vector3::UnitX };

		float totalPitch{ 0.f };
		float totalYaw{ 0.f };

		Matrix cameraToWorld{};


		Matrix CalculateCameraToWorld()
		{
			right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			up = Vector3::Cross(forward, right).Normalized();
			

			cameraToWorld = Matrix(Vector4(right, 0.f), Vector4(up, 0.f), Vector4(forward, 0.f), Vector4(origin, 1.f));

			return cameraToWorld;
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			Vector3 velocity { 10.f, 10.f, 10.f };
			const float rotationVeloctiy{ 0.1f * PI / 180.0f};

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);


			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			if (pKeyboardState[SDL_SCANCODE_W]) origin += forward * velocity.z * deltaTime;
			if (pKeyboardState[SDL_SCANCODE_S]) origin -= forward * velocity.z * deltaTime;
			if (pKeyboardState[SDL_SCANCODE_A]) origin -= right * velocity.x * deltaTime;
			if (pKeyboardState[SDL_SCANCODE_D]) origin += right * velocity.x * deltaTime;


			bool leftButtonPressed = mouseState & SDL_BUTTON(SDL_BUTTON_LEFT);
			bool rightButtonPressed = mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT);

			if (leftButtonPressed)
			{
				origin += forward * mouseY * deltaTime;
				totalPitch += mouseX * rotationVeloctiy;
			}
			if (rightButtonPressed)
			{
				totalPitch += mouseX * rotationVeloctiy;
				totalYaw += mouseY * rotationVeloctiy;
			}

			Matrix finalRotation;

			finalRotation = finalRotation.CreateRotation(totalYaw, totalPitch, 0.f);
			forward = finalRotation.TransformVector(Vector3::UnitZ);
			forward.Normalize();
		
		}
	};
}

#pragma once
#include <fstream>
#include "Maths.h"
#include "DataTypes.h"

namespace dae
{
	namespace GeometryUtils
	{
#pragma region Sphere HitTest
		//SPHERE HIT-TESTS
		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			float A{ Vector3::Dot(ray.direction, ray.direction) };

			Vector3 vectorFromStoR{ sphere.origin, ray.origin };
			float B{  Vector3::Dot(2 * ray.direction , vectorFromStoR) };

			float C{ Vector3::Dot(vectorFromStoR,  vectorFromStoR) - sphere.radius * sphere.radius};

			float D{ B * B - 4 * A * C };

			//todo W1
			//throw std::runtime_error("Not Implemented Yet");
			if (D > 0) 
			{
				float t{ (-B - sqrtf(D)) / (2 * A) };
				if (t < ray.min)
				{
					t = (-B + sqrtf(D)) / (2 * A);
				}

				if (t >= ray.min && t < ray.max)
				{
					if (ignoreHitRecord == false)
					{
						hitRecord.didHit = true;
						hitRecord.materialIndex = sphere.materialIndex;
						hitRecord.t = t;
						hitRecord.origin = Vector3{ ray.origin + hitRecord.t * ray.direction };
						hitRecord.normal = (hitRecord.origin - sphere.origin).Normalized();
					}
					return true;
				}
			}
			hitRecord.didHit = false;
			return false;
		}

		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Sphere(sphere, ray, temp, true);
		}
#pragma endregion
#pragma region Plane HitTest
		//PLANE HIT-TESTS
		inline bool HitTest_Plane(const Plane& plane, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			//todo W1
			//throw std::runtime_error("Not Implemented Yet");
			float t{ Vector3::Dot(Vector3(plane.origin - ray.origin), plane.normal) / Vector3::Dot(ray.direction, plane.normal) };

			if ((t >= ray.min) && (t < ray.max))
			{
				if (ignoreHitRecord == false)
				{
					hitRecord.t = t;
					hitRecord.didHit = true;
					hitRecord.materialIndex = plane.materialIndex;
					hitRecord.normal = plane.normal;
					hitRecord.origin = ray.origin + hitRecord.t * ray.direction;
				}
				return true;
				
	
			}
			hitRecord.didHit = false;
			return false;
		}

		inline bool HitTest_Plane(const Plane& plane, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Plane(plane, ray, temp, true);
		}
#pragma endregion
#pragma region Triangle HitTest
		//TRIANGLE HIT-TESTS
		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			//todo W5
			auto a = triangle.v1 - triangle.v0;
			auto b = triangle.v2 - triangle.v0;

			auto n = Vector3::Cross(a, b).Normalized();

			TriangleCullMode cullMode = triangle.cullMode;
			if (ignoreHitRecord)
			{
				
				if (triangle.cullMode == TriangleCullMode::FrontFaceCulling) cullMode = TriangleCullMode::BackFaceCulling;
				else if (triangle.cullMode == TriangleCullMode::BackFaceCulling) cullMode = TriangleCullMode::FrontFaceCulling;
			}

			if (Vector3::Dot(n, ray.direction) > 0.f && cullMode == TriangleCullMode::BackFaceCulling)
			{
				return false;
			}

			if (Vector3::Dot(n, ray.direction) < 0.f && cullMode == TriangleCullMode::FrontFaceCulling)
			{
				return false;
			}

			if (AreEqual(Vector3::Dot(n, ray.direction), 0))
			{
				return false;
			} 
			else
			{
				auto L = triangle.v0 - ray.origin;

				auto t = Vector3::Dot(L, n) / Vector3::Dot(ray.direction, n);

				if ((t < ray.min) || (t > ray.max))
				{
					return false;
				}
				else
				{
					auto P = ray.origin + ray.direction * t;

					auto e1 = triangle.v1 - triangle.v0;
					auto p1 = P - triangle.v0;

					auto e2 = triangle.v2 - triangle.v1;
					auto p2 = P - triangle.v1;

					auto e3 = triangle.v0 - triangle.v2;
					auto p3 = P - triangle.v2;

					if (Vector3::Dot(Vector3::Cross(e1, p1), n) < 0.f || Vector3::Dot(Vector3::Cross(e2, p2), n) < 0.f || Vector3::Dot(Vector3::Cross(e3, p3), n) < 0.f)
					{
						return false;
					} 
					else
					{
						

						hitRecord.didHit = true;
						hitRecord.t = t;
						hitRecord.normal = n;
						hitRecord.materialIndex = triangle.materialIndex;
						hitRecord.origin = P;
						return true;
					}
				}
			}
			return false;
		}

		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Triangle(triangle, ray, temp, true);
		}
#pragma endregion
#pragma region TriangeMesh HitTest
		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			HitRecord minHitRecord;

			for (int inx = 0; inx < mesh.indices.size(); inx += 3)
			{
				auto newTriangle = Triangle(mesh.transformedPositions[mesh.indices[inx]], mesh.transformedPositions[mesh.indices[inx + 1]], mesh.transformedPositions[mesh.indices[inx + 2]], mesh.transformedNormals[inx / 3]);
				newTriangle.cullMode = mesh.cullMode;
				newTriangle.materialIndex = mesh.materialIndex;

				HitTest_Triangle(newTriangle, ray, hitRecord, ignoreHitRecord);
				if (inx == 0 || (inx > 0 && hitRecord.t < minHitRecord.t))
				{
					minHitRecord = hitRecord;
				}

			}
			hitRecord = minHitRecord;
			return hitRecord.didHit;
		}

		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_TriangleMesh(mesh, ray, temp, true);
		}
#pragma endregion
	}

	namespace LightUtils
	{
		//Direction from target to light
		inline Vector3 GetDirectionToLight(const Light& light, const Vector3 origin)
		{
			//todo W3
			//throw std::runtime_error("Not Implemented Yet");
			return Vector3(origin, light.origin);
		}

		inline ColorRGB GetRadiance(const Light& light, const Vector3& target)
		{
			//todo W3
			if (light.type == LightType::Point)
			{

				return light.color * (light.intensity / Vector3::Dot((light.origin - target), (light.origin - target)));
			
			}

			if (light.type == LightType::Directional)
			{
				return light.color * light.intensity;
			}
			return ColorRGB{};
		} 
	}

	namespace Utils
	{
		//Just parses vertices and indices
#pragma warning(push)
#pragma warning(disable : 4505) //Warning unreferenced local function
		static bool ParseOBJ(const std::string& filename, std::vector<Vector3>& positions, std::vector<Vector3>& normals, std::vector<int>& indices)
		{
			std::ifstream file(filename);
			if (!file)
				return false;

			std::string sCommand;
			// start a while iteration ending when the end of file is reached (ios::eof)
			while (!file.eof())
			{
				//read the first word of the string, use the >> operator (istream::operator>>) 
				file >> sCommand;
				//use conditional statements to process the different commands	
				if (sCommand == "#")
				{
					// Ignore Comment
				}
				else if (sCommand == "v")
				{
					//Vertex
					float x, y, z;
					file >> x >> y >> z;
					positions.push_back({ x, y, z });
				}
				else if (sCommand == "f")
				{
					float i0, i1, i2;
					file >> i0 >> i1 >> i2;

					indices.push_back((int)i0 - 1);
					indices.push_back((int)i1 - 1);
					indices.push_back((int)i2 - 1);
				}
				//read till end of line and ignore all remaining chars
				file.ignore(1000, '\n');

				if (file.eof())
					break;
			}

			//Precompute normals
			for (uint64_t index = 0; index < indices.size(); index += 3)
			{
				uint32_t i0 = indices[index];
				uint32_t i1 = indices[index + 1];
				uint32_t i2 = indices[index + 2];

				Vector3 edgeV0V1 = positions[i1] - positions[i0];
				Vector3 edgeV0V2 = positions[i2] - positions[i0];
				Vector3 normal = Vector3::Cross(edgeV0V1, edgeV0V2);

				if (std::isnan(normal.x))
				{
					int k = 0;
				}

				normal.Normalize();
				if (std::isnan(normal.x))
				{
					int k = 0;
				}

				normals.push_back(normal);
			}

			return true;
		}
#pragma warning(pop)
	}
}
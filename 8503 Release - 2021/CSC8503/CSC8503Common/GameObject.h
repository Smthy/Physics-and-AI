#pragma once
#include "Transform.h"
#include "CollisionVolume.h"

#include "PhysicsObject.h"
#include "RenderObject.h"

#include <vector>

using std::vector;

namespace NCL {
	namespace CSC8503 {

		class GameObject	{
		public:
			GameObject(string name = "");
			~GameObject();

			void SetBoundingVolume(CollisionVolume* vol) {
				boundingVolume = vol;
			}

			const CollisionVolume* GetBoundingVolume() const {
				return boundingVolume;
			}

			bool IsActive() const {
				return isActive;
			}

			Transform& GetTransform() {
				return transform;
			}

			RenderObject* GetRenderObject() const {
				return renderObject;
			}

			PhysicsObject* GetPhysicsObject() const {
				return physicsObject;
			}			

			void SetRenderObject(RenderObject* newObject) {
				renderObject = newObject;
			}

			void SetPhysicsObject(PhysicsObject* newObject) {
				physicsObject = newObject;
			}
			
			void SetName(string newName) {
				name = newName;
			}

			void SetColor(Vector4 newColor) {
				color = newColor;
			}

			const Vector4 GetColor() const {
				return color;
			}

			const string& GetName() const {
				return name;
			}

			
			virtual void OnCollisionBegin(GameObject* otherObject) {
				//std::cout << "OnCollisionBegin event occured!\n";
				if (this->GetName() == "Ball" && otherObject->GetName() == "Cube_21") {
					
					std::cout << "END GAME" << std::endl;					
				}
				if (this->GetName() == "Ball" && otherObject->GetName() == "floor") {
					std::cout << "Hit floor" << std::endl;
					this->GetTransform().SetPosition(Vector3(0, 25, 0));
				}
			}

			virtual void OnCollisionEnd(GameObject* otherObject) {
				//std::cout << "OnCollisionEnd event occured!\n";
			}

			virtual void OnCollisionStay(GameObject* otherObject) {
				//std::cout << "OnCollisionBegin event occured!\n";
			}

			bool GetBroadphaseAABB(Vector3&outsize) const;

			void UpdateBroadphaseAABB();

			void SetWorldID(int newID) {
				worldID = newID;
			}

			int		GetWorldID() const {
				return worldID;
			}

		protected:
			Transform			transform;

			CollisionVolume*	boundingVolume;
			PhysicsObject*		physicsObject;
			RenderObject*		renderObject;

			bool	isActive;
			int		worldID;
			string	name;
			Vector4 color;			

			Vector3 broadphaseAABB;
		};
	}
}


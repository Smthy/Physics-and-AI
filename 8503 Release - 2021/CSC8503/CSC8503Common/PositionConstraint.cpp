#include "PositionConstraint.h"

using namespace NCL;
using namespace CSC8503;


/*
void PositionConstraint::UpdateConstraint(float dt) {
	Vector3 relativePos = objectA->GetTransform().GetPosition() - objectB->GetTransform().GetPosition();
	float currentDistance = relativePos.Length();
	float offset = distance - currentDistance;

	if (abs(offset) > 0.0f) {
		Vector3 offsetDir = relativePos.Normalised();

		PhysicsObject* physA = objectA->GetPhysicsObject();
		PhysicsObject* physB = objectB->GetPhysicsObject();
		Vector3 relativeVelocity = physA->GetLinearVelocity() -
			physB->GetLinearVelocity();
		float constraintMass = physA->GetInverseMass() +
			physB->GetInverseMass();
		if (constraintMass > 0.0f) {
			float velocityDot = Vector3::Dot(relativeVelocity, offsetDir);
			float biasFactor = 0.01f;
			float bias = -(biasFactor / dt) * offset;
			float lambda = -(velocityDot + bias) / constraintMass;
			Vector3 aImpulse = offsetDir * lambda;
			Vector3 bImpulse = -offsetDir * lambda;
			physA->ApplyLinearImpulse(aImpulse);// multiplied by mass here
			physB->ApplyLinearImpulse(bImpulse);// multiplied by mass here
		}
	}
}
*/

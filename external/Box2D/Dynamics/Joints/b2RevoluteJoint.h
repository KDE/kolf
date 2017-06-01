/*
* Copyright (c) 2006-2007 Erin Catto http://www.gphysics.com
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef B2_REVOLUTE_JOINT_H
#define B2_REVOLUTE_JOINT_H

#include <Box2D/Dynamics/Joints/b2Joint.h>

/// Revolute joint definition. This requires defining an
/// anchor point where the bodies are joined. The definition
/// uses local anchor points so that the initial configuration
/// can violate the constraint slightly. You also need to
/// specify the initial relative angle for joint limits. This
/// helps when saving and loading a game.
/// The local anchor points are measured from the body's origin
/// rather than the center of mass because:
/// 1. you might not know where the center of mass will be.
/// 2. if you add/remove shapes from a body and recompute the mass,
///    the joints will be broken.
struct b2RevoluteJointDef : public b2JointDef
{
	b2RevoluteJointDef()
	{
		type = e_revoluteJoint;
		localAnchorA.Set(0.0f, 0.0f);
		localAnchorB.Set(0.0f, 0.0f);
		referenceAngle = 0.0f;
		lowerAngle = 0.0f;
		upperAngle = 0.0f;
		maxMotorTorque = 0.0f;
		motorSpeed = 0.0f;
		enableLimit = false;
		enableMotor = false;
	}

	/// Initialize the bodies, anchors, and reference angle using a world
	/// anchor point.
	void Initialize(b2Body* bodyA, b2Body* bodyB, const b2Vec2& anchor);

	/// The local anchor point relative to body1's origin.
	b2Vec2 localAnchorA;

	/// The local anchor point relative to body2's origin.
	b2Vec2 localAnchorB;

	/// The body2 angle minus body1 angle in the reference state (radians).
	qreal referenceAngle;

	/// A flag to enable joint limits.
	bool enableLimit;

	/// The lower angle for the joint limit (radians).
	qreal lowerAngle;

	/// The upper angle for the joint limit (radians).
	qreal upperAngle;

	/// A flag to enable the joint motor.
	bool enableMotor;

	/// The desired motor speed. Usually in radians per second.
	qreal motorSpeed;

	/// The maximum motor torque used to achieve the desired motor speed.
	/// Usually in N-m.
	qreal maxMotorTorque;
};

/// A revolute joint constrains two bodies to share a common point while they
/// are free to rotate about the point. The relative rotation about the shared
/// point is the joint angle. You can limit the relative rotation with
/// a joint limit that specifies a lower and upper angle. You can use a motor
/// to drive the relative rotation about the shared point. A maximum motor torque
/// is provided so that infinite forces are not generated.
class b2RevoluteJoint : public b2Joint
{
public:
	b2Vec2 GetAnchorA() const Q_DECL_OVERRIDE;
	b2Vec2 GetAnchorB() const Q_DECL_OVERRIDE;

	/// Get the current joint angle in radians.
	qreal GetJointAngle() const;

	/// Get the current joint angle speed in radians per second.
	qreal GetJointSpeed() const;

	/// Is the joint limit enabled?
	bool IsLimitEnabled() const;

	/// Enable/disable the joint limit.
	void EnableLimit(bool flag);

	/// Get the lower joint limit in radians.
	qreal GetLowerLimit() const;

	/// Get the upper joint limit in radians.
	qreal GetUpperLimit() const;

	/// Set the joint limits in radians.
	void SetLimits(qreal lower, qreal upper);

	/// Is the joint motor enabled?
	bool IsMotorEnabled() const;

	/// Enable/disable the joint motor.
	void EnableMotor(bool flag);

	/// Set the motor speed in radians per second.
	void SetMotorSpeed(qreal speed);

	/// Get the motor speed in radians per second.
	qreal GetMotorSpeed() const;

	/// Set the maximum motor torque, usually in N-m.
	void SetMaxMotorTorque(qreal torque);

	/// Get the reaction force given the inverse time step.
	/// Unit is N.
	b2Vec2 GetReactionForce(qreal inv_dt) const Q_DECL_OVERRIDE;

	/// Get the reaction torque due to the joint limit given the inverse time step.
	/// Unit is N*m.
	qreal GetReactionTorque(qreal inv_dt) const Q_DECL_OVERRIDE;

	/// Get the current motor torque given the inverse time step.
	/// Unit is N*m.
	qreal GetMotorTorque(qreal inv_dt) const;

protected:
	
	friend class b2Joint;
	friend class b2GearJoint;

	b2RevoluteJoint(const b2RevoluteJointDef* def);

	void InitVelocityConstraints(const b2TimeStep& step) Q_DECL_OVERRIDE;
	void SolveVelocityConstraints(const b2TimeStep& step) Q_DECL_OVERRIDE;

	bool SolvePositionConstraints(qreal baumgarte) Q_DECL_OVERRIDE;

	b2Vec2 m_localAnchor1;	// relative
	b2Vec2 m_localAnchor2;
	b2Vec3 m_impulse;
	qreal m_motorImpulse;

	b2Mat33 m_mass;			// effective mass for point-to-point constraint.
	qreal m_motorMass;	// effective mass for motor/limit angular constraint.
	
	bool m_enableMotor;
	qreal m_maxMotorTorque;
	qreal m_motorSpeed;

	bool m_enableLimit;
	qreal m_referenceAngle;
	qreal m_lowerAngle;
	qreal m_upperAngle;
	b2LimitState m_limitState;
};

inline qreal b2RevoluteJoint::GetMotorSpeed() const
{
	return m_motorSpeed;
}

#endif

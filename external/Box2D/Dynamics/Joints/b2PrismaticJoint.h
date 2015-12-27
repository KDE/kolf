/*
* Copyright (c) 2006-2011 Erin Catto http://www.box2d.org
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

#ifndef B2_PRISMATIC_JOINT_H
#define B2_PRISMATIC_JOINT_H

#include <Box2D/Dynamics/Joints/b2Joint.h>

/// Prismatic joint definition. This requires defining a line of
/// motion using an axis and an anchor point. The definition uses local
/// anchor points and a local axis so that the initial configuration
/// can violate the constraint slightly. The joint translation is zero
/// when the local anchor points coincide in world space. Using local
/// anchors and a local axis helps when saving and loading a game.
struct b2PrismaticJointDef : public b2JointDef
{
	b2PrismaticJointDef()
	{
		type = e_prismaticJoint;
		localAnchorA.SetZero();
		localAnchorB.SetZero();
		localAxisA.Set(1.0f, 0.0f);
		referenceAngle = 0.0f;
		enableLimit = false;
		lowerTranslation = 0.0f;
		upperTranslation = 0.0f;
		enableMotor = false;
		maxMotorForce = 0.0f;
		motorSpeed = 0.0f;
	}

	/// Initialize the bodies, anchors, axis, and reference angle using the world
	/// anchor and unit world axis.
	void Initialize(b2Body* bodyA, b2Body* bodyB, const b2Vec2& anchor, const b2Vec2& axis);

	/// The local anchor point relative to bodyA's origin.
	b2Vec2 localAnchorA;

	/// The local anchor point relative to bodyB's origin.
	b2Vec2 localAnchorB;

	/// The local translation unit axis in bodyA.
	b2Vec2 localAxisA;

	/// The constrained angle between the bodies: bodyB_angle - bodyA_angle.
	qreal referenceAngle;

	/// Enable/disable the joint limit.
	bool enableLimit;

	/// The lower translation limit, usually in meters.
	qreal lowerTranslation;

	/// The upper translation limit, usually in meters.
	qreal upperTranslation;

	/// Enable/disable the joint motor.
	bool enableMotor;

	/// The maximum motor torque, usually in N-m.
	qreal maxMotorForce;

	/// The desired motor speed in radians per second.
	qreal motorSpeed;
};

/// A prismatic joint. This joint provides one degree of freedom: translation
/// along an axis fixed in bodyA. Relative rotation is prevented. You can
/// use a joint limit to restrict the range of motion and a joint motor to
/// drive the motion or to model joint friction.
class b2PrismaticJoint : public b2Joint
{
public:
	b2Vec2 GetAnchorA() const;
	b2Vec2 GetAnchorB() const;

	b2Vec2 GetReactionForce(qreal inv_dt) const;
	qreal GetReactionTorque(float32 inv_dt) const;

	/// The local anchor point relative to bodyA's origin.
	const b2Vec2& GetLocalAnchorA() const { return m_localAnchorA; }

	/// The local anchor point relative to bodyB's origin.
	const b2Vec2& GetLocalAnchorB() const  { return m_localAnchorB; }

	/// The local joint axis relative to bodyA.
	const b2Vec2& GetLocalAxisA() const { return m_localXAxisA; }

	/// Get the reference angle.
	qreal GetReferenceAngle() const { return m_referenceAngle; }

	/// Get the current joint translation, usually in meters.
	qreal GetJointTranslation() const;

	/// Get the current joint translation speed, usually in meters per second.
	qreal GetJointSpeed() const;

	/// Is the joint limit enabled?
	bool IsLimitEnabled() const;

	/// Enable/disable the joint limit.
	void EnableLimit(bool flag);

	/// Get the lower joint limit, usually in meters.
	qreal GetLowerLimit() const;

	/// Get the upper joint limit, usually in meters.
	qreal GetUpperLimit() const;

	/// Set the joint limits, usually in meters.
	void SetLimits(qreal lower, float32 upper);

	/// Is the joint motor enabled?
	bool IsMotorEnabled() const;

	/// Enable/disable the joint motor.
	void EnableMotor(bool flag);

	/// Set the motor speed, usually in meters per second.
	void SetMotorSpeed(qreal speed);

	/// Get the motor speed, usually in meters per second.
	qreal GetMotorSpeed() const;

	/// Set the maximum motor force, usually in N.
	void SetMaxMotorForce(qreal force);
	qreal GetMaxMotorForce() const { return m_maxMotorForce; }

	/// Get the current motor force given the inverse time step, usually in N.
	qreal GetMotorForce(float32 inv_dt) const;

	/// Dump to b2Log
	void Dump();

protected:
	friend class b2Joint;
	friend class b2GearJoint;
	b2PrismaticJoint(const b2PrismaticJointDef* def);

	void InitVelocityConstraints(const b2SolverData& data);
	void SolveVelocityConstraints(const b2SolverData& data);
	bool SolvePositionConstraints(const b2SolverData& data);

	// Solver shared
	b2Vec2 m_localAnchorA;
	b2Vec2 m_localAnchorB;
	b2Vec2 m_localXAxisA;
	b2Vec2 m_localYAxisA;
	qreal m_referenceAngle;
	b2Vec3 m_impulse;
	qreal m_motorImpulse;
	qreal m_lowerTranslation;
	qreal m_upperTranslation;
	qreal m_maxMotorForce;
	qreal m_motorSpeed;
	bool m_enableLimit;
	bool m_enableMotor;
	b2LimitState m_limitState;

	// Solver temp
	int32 m_indexA;
	int32 m_indexB;
	b2Vec2 m_localCenterA;
	b2Vec2 m_localCenterB;
	qreal m_invMassA;
	qreal m_invMassB;
	qreal m_invIA;
	qreal m_invIB;
	b2Vec2 m_axis, m_perp;
	qreal m_s1, m_s2;
	qreal m_a1, m_a2;
	b2Mat33 m_K;
	qreal m_motorMass;
};

inline qreal b2PrismaticJoint::GetMotorSpeed() const
{
	return m_motorSpeed;
}

#endif

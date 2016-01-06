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

#ifndef B2_DISTANCE_JOINT_H
#define B2_DISTANCE_JOINT_H

#include <Box2D/Dynamics/Joints/b2Joint.h>

/// Distance joint definition. This requires defining an
/// anchor point on both bodies and the non-zero length of the
/// distance joint. The definition uses local anchor points
/// so that the initial configuration can violate the constraint
/// slightly. This helps when saving and loading a game.
/// @warning Do not use a zero or short length.
struct b2DistanceJointDef : public b2JointDef
{
	b2DistanceJointDef()
	{
		type = e_distanceJoint;
		localAnchorA.Set(0.0f, 0.0f);
		localAnchorB.Set(0.0f, 0.0f);
		length = 1.0f;
		frequencyHz = 0.0f;
		dampingRatio = 0.0f;
	}

	/// Initialize the bodies, anchors, and length using the world
	/// anchors.
	void Initialize(b2Body* bodyA, b2Body* bodyB,
					const b2Vec2& anchorA, const b2Vec2& anchorB);

	/// The local anchor point relative to body1's origin.
	b2Vec2 localAnchorA;

	/// The local anchor point relative to body2's origin.
	b2Vec2 localAnchorB;

	/// The natural length between the anchor points.
	qreal length;

	/// The mass-spring-damper frequency in Hertz.
	qreal frequencyHz;

	/// The damping ratio. 0 = no damping, 1 = critical damping.
	qreal dampingRatio;
};

/// A distance joint constrains two points on two bodies
/// to remain at a fixed distance from each other. You can view
/// this as a massless, rigid rod.
class b2DistanceJoint : public b2Joint
{
public:

	b2Vec2 GetAnchorA() const;
	b2Vec2 GetAnchorB() const;

	/// Get the reaction force given the inverse time step.
	/// Unit is N.
	b2Vec2 GetReactionForce(qreal inv_dt) const;

	/// Get the reaction torque given the inverse time step.
	/// Unit is N*m. This is always zero for a distance joint.
	qreal GetReactionTorque(qreal inv_dt) const;

	/// Set/get the natural length.
	/// Manipulating the length can lead to non-physical behavior when the frequency is zero.
	void SetLength(qreal length);
	qreal GetLength() const;

	// Set/get frequency in Hz.
	void SetFrequency(qreal hz);
	qreal GetFrequency() const;

	// Set/get damping ratio.
	void SetDampingRatio(qreal ratio);
	qreal GetDampingRatio() const;

protected:

	friend class b2Joint;
	b2DistanceJoint(const b2DistanceJointDef* data);

	void InitVelocityConstraints(const b2TimeStep& step);
	void SolveVelocityConstraints(const b2TimeStep& step);
	bool SolvePositionConstraints(qreal baumgarte);

	b2Vec2 m_localAnchor1;
	b2Vec2 m_localAnchor2;
	b2Vec2 m_u;
	qreal m_frequencyHz;
	qreal m_dampingRatio;
	qreal m_gamma;
	qreal m_bias;
	qreal m_impulse;
	qreal m_mass;
	qreal m_length;
};

inline void b2DistanceJoint::SetLength(qreal length)
{
	m_length = length;
}

inline qreal b2DistanceJoint::GetLength() const
{
	return m_length;
}

inline void b2DistanceJoint::SetFrequency(qreal hz)
{
	m_frequencyHz = hz;
}

inline qreal b2DistanceJoint::GetFrequency() const
{
	return m_frequencyHz;
}

inline void b2DistanceJoint::SetDampingRatio(qreal ratio)
{
	m_dampingRatio = ratio;
}

inline qreal b2DistanceJoint::GetDampingRatio() const
{
	return m_dampingRatio;
}

#endif

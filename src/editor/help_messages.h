#pragma once

#define S(n) constexpr const char* const (n)

namespace help {

S(emissionType) = "The shape in which particles are emitted";
S(drawType) = "The way the particle is rendered";
S(texture) = "The texture that the particle will use";
S(emissionAxis) = "For oriented emission shapes, this gives the axis of these shapes";
S(hasRotation) = "Whether particles should rotate";
S(randomInitAngle) = "Whether particles should have a randomized initial angle";
S(selfMaintaining) = R"(
Whether the emitter manages itself or not.
If set, the emitter will automatically terminate when it reaches the end of its life
and all of its particles have died
)";
S(followEmitter) = "Whether particles should continuously move relative to the emitter";
S(polygonRotAxis) = "The axis to rotate the polygon around when using the 'polygon' draw types";
S(polygonReferencePlane) = "The base plane used for the 'polygon' draw types";
S(randomizeLoopedAnim) = "Whether to randomize the starting point of looped animations";
S(drawChildrenFirst) = "If set, child particles will be rendered before parent particles";
S(hideParent) = "If set, only child particles will be rendered";
S(useViewSpace) = "If set, the rendering calculations will be done in view space";
S(hasFixedPolygonID) = "Whether the particles use the fixed polygon ID";
S(childHasFixedPolygonID) = "Whether child particles use the fixed polygon ID";

S(emitterBasePos) = "Base world-space position of the emitter";
S(emissionCount) = "Number of particles emitted per emission";
S(radius) = "Specifies the radius for Circle, Sphere, and Cylinder emission types";
S(length) = "Specifies the length for the Cylinder emission type";
S(axis) = "Specifies the axis of Circle, Cylinder, and Hemisphere emission types";
S(color) = "The albedo color of particles";
S(initVelPosAmplifier) = "Initial Velocity Amplifier based on emitter position";
S(initVelAxisAmplifier) = "Initial Velocity Amplifier based on emitter axis";
S(baseScale) = "The base scale of particles";
S(aspectRatio) = "The aspect ratio of particles (affects X scaling)";
S(startDelay) = "The time, in seconds, until the emitter starts emitting particles";
S(rotationSpeed) = "The minimum/maximum initial angular velocity of particles";
S(initAngle) = "The initial angle of particles";
S(emitterLifeTime) = "The time, in seconds, the emitter will live for";
S(particleLifeTime) = "The time, in seconds, the particles will live for";

S(variance) = "Variance Factors that scale the random parameters of particles";

S(emissionInterval) = "The time, in seconds, between particle emissions";
S(emissions) = "The number of emissions the emitter will do across its lifespan";
S(baseAlpha) = "The base alpha value of particles";
S(airResistance) = "The air resistance applied to particles (lower = more resistance)";
S(loopTime) = "The time, in seconds, a particle animation loop takes";
S(loops) = "The number of times a particle should loop its animations across its lifespan";
S(dbbScale) = "Scaling Factor for the Directional Billboard draw type";
S(textureTiling) = "The number of times a texture is tiled across the particles surface. Only powers of 2 are allowed";
S(scaleAnimDir) = "The axis/axes affected scale animations";
S(dpolFaceEmitter) = "If set, the polygon will face the emitter when using the Directional Polygon draw type";
S(flipTextureX) = "Whether to flip the texture on the X axis";
S(flipTextureY) = "Whether to flip the texture on the Y axis";
S(polygonOffset) = "An offset, in model-space, for the particle polygon";

S(childEmissionDelay) = "The delay, as a fraction of the particles lifetime, until it starts emitting children";
S(childEmissionInterval) = "The time, in seconds, between child particle emissions";
S(childEmissions) = "The number of emissions the particle will do across its lifespan";
S(childRotation) = "Specifies the rotation attributes inherited from the parent";
S(childTexture) = "The texture that the child particles will use";
S(usesBehaviors) = "Whether the child particle uses behaviors (it will use the behaviors of the parent)";
S(randomInitVelMag) = "Specifies a range [-n, n) of random values which will be added to the initial velocity";
S(velocityRatio) = "The ratio of the velocity inherited from the parent";
S(scaleRatio) = "The ratio of the scale inherited from the parent";
S(useChildColor) = "Whether to use the color specified above. Otherwise the parent particles color will be inherited";
S(hasScaleAnim) = "Whether child particles use a scale animation";
S(endScale) = "The final scale value the child particles grow to";
S(hasAlphaAnim) = "Whether child particles have a fade-out animation";

}

#undef S

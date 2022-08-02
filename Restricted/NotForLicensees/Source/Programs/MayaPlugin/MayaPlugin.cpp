// MIT License

// Copyright (c) 2022 Autodesk, Inc.

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "RequiredProgramMainCPPInclude.h"
#include "Misc/CommandLine.h"
#include "CoreMinimal.h"
#include <cstdlib>
#include <set>
IMPLEMENT_APPLICATION(MayaPlugin, "MayaPlugin");

#include "MayaCommonIncludes.h"
#include "MayaUtils.h"


THIRD_PARTY_INCLUDES_START
#include <maya/MAnimMessage.h>
#ifndef MAYA_OLD_PLUGIN
#include <maya/MCameraMessage.h>
#endif // MAYA_OLD_PLUGIN
THIRD_PARTY_INCLUDES_END

#define MCHECKERROR(STAT,MSG)	\
	if (!STAT) {				\
		perror(MSG);			\
		return MS::kFailure;	\
	}

#define MREPORTERROR(STAT,MSG)	\
	if (!STAT) {				\
		perror(MSG);			\
	}

MSpace::Space G_TransformSpace = MSpace::kTransform;
bool bUEInitialized = false;

const char PluginVersion[] = "v1.1.1";
const char PluginAppId[] = "3726213941804942083";

const MString LiveLinkAddSelectionCommandName("uePosition");

class LiveLinkAddSelectionCommand : public MPxCommand
{
public:
	static void		cleanup() {}
	static void*	creator() { return new LiveLinkAddSelectionCommand(); }

	MStatus			doIt(const MArgList& args) override
	{
		MStatus stat;
		MSelectionList SelectedItems;
		MGlobal::getActiveSelectionList(SelectedItems);
		MDagPath source, target;
		MObject tempObject;
		stat = SelectedItems.getDagPath(0, source, tempObject);
		if (!stat)
			return MS::kFailure;
		MFnCamera C(source);
		MPoint EyeLocation = C.eyePoint(MSpace::kWorld);
		const auto SceneTime = MayaUtils::GetMayaFrameTimeAsUnrealTime();
		MMatrix CameraTransformMatrix;
		MayaUtils::SetMatrixRow(CameraTransformMatrix[0], C.rightDirection(MSpace::kWorld));
		MayaUtils::SetMatrixRow(CameraTransformMatrix[1], C.viewDirection(MSpace::kWorld));
		MayaUtils::SetMatrixRow(CameraTransformMatrix[2], C.upDirection(MSpace::kWorld));
		MayaUtils::SetMatrixRow(CameraTransformMatrix[3], EyeLocation);
		MayaUtils::RotateCoordinateSystemForUnreal(CameraTransformMatrix);
		FTransform CameraTransform = MayaUtils::BuildUETransformFromMayaTransform(CameraTransformMatrix);
		// Convert Maya Camera orientation to Unreal
		CameraTransform.SetRotation(CameraTransform.GetRotation() * FRotator(0.0f, -90.0f, 0.0f).Quaternion());
		FVector MoveBy = CameraTransform.GetLocation();
		FVector euler = CameraTransform.GetRotation().Euler();
		FRotator rotator = FRotator::MakeFromEuler(euler);
		MGlobal::displayInfo(MString(" tx ") + MoveBy.X + MString(" ty ") + MoveBy.Y + MString(" tz ") + MoveBy.Z);
		MGlobal::displayInfo(MString(" rx ")+ rotator.Roll + MString(" ry ") + rotator.Pitch + MString(" rz ") + rotator.Yaw  );
		appendToResult(MoveBy.X);
		appendToResult(MoveBy.Y);
		appendToResult(MoveBy.Z);
		appendToResult(rotator.Roll);
		appendToResult(rotator.Pitch);
		appendToResult(rotator.Yaw);
		return MS::kSuccess;
	}
};

#if PLATFORM_WINDOWS
#define EXTERN_DECL __declspec( dllexport )
#else
#define EXTERN_DECL extern
#endif


EXTERN_DECL MStatus initializePlugin(MObject MayaPluginObject)
{
	MFnPlugin MayaPlugin(MayaPluginObject,"MayaPlugin",PluginVersion);
	MayaPlugin.registerCommand(LiveLinkAddSelectionCommandName, LiveLinkAddSelectionCommand::creator);
	const MStatus MayaStatusResult = MS::kSuccess;
	return MayaStatusResult;
}


EXTERN_DECL MStatus uninitializePlugin(MObject MayaPluginObject)
{
	MFnPlugin MayaPlugin(MayaPluginObject);
	MayaPlugin.deregisterCommand(LiveLinkAddSelectionCommandName);
	const MStatus MayaStatusResult = MS::kSuccess;
	return MayaStatusResult;
}

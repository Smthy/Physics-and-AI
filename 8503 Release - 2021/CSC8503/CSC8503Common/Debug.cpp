#include "Debug.h"
#include "../../Common/Matrix4.h"
using namespace NCL;

OGLRenderer* Debug::renderer = nullptr;

std::vector<Debug::DebugStringEntry>	Debug::stringEntries;
std::vector<Debug::DebugLineEntry>		Debug::lineEntries;

const Vector4 Debug::RED	= Vector4(1, 0, 0, 1);
const Vector4 Debug::GREEN	= Vector4(0, 1, 0, 1);
const Vector4 Debug::BLUE	= Vector4(0, 0, 1, 1);

const Vector4 Debug::BLACK	= Vector4(0, 0, 0, 1);
const Vector4 Debug::WHITE	= Vector4(1, 1, 1, 1);

const Vector4 Debug::YELLOW		= Vector4(1, 1, 0, 1);
const Vector4 Debug::MAGENTA	= Vector4(1, 0, 1, 1);
const Vector4 Debug::CYAN		= Vector4(0, 1, 1, 1);

const Vector4 Debug::DARKBLUE = Vector4(0.05, 0.47, 0.62, 1);
const Vector4 Debug::DARKGREEN = Vector4(0.02, 0.56, 0.14, 1);
const Vector4 Debug::ORANGE = Vector4(0.98, 0.45, 0, 1);
const Vector4 Debug::DARKPURPLE = Vector4(0.51, 0.39, 0.63, 1);
const Vector4 Debug::DARKRED = Vector4(0.58, 0.02, 0.02, 1);
const Vector4 Debug::TURQUOISE = Vector4(0, 1, 1, 0.5);


void Debug::Print(const std::string& text, const Vector2&pos, const Vector4& color) {
	
	DebugStringEntry newEntry;

	newEntry.data		= text;
	newEntry.position	= pos;
	newEntry.color		= color;

	stringEntries.emplace_back(newEntry);
}

void Debug::DrawLine(const Vector3& startpoint, const Vector3& endpoint, const Vector4& color, float time) {
	DebugLineEntry newEntry;

	newEntry.start	= startpoint;
	newEntry.end	= endpoint;
	newEntry.color = color;
	newEntry.time	= time;

	lineEntries.emplace_back(newEntry);
}

void Debug::DrawAxisLines(const Matrix4& modelMatrix, float scaleBoost, float time) {
	Matrix4 local = modelMatrix;
	local.SetPositionVector({0, 0, 0});

	Vector3 fwd		= local * Vector4(0, 0, -1, 1.0f);
	Vector3 up		= local * Vector4(0, 1, 0, 1.0f);
	Vector3 right	= local * Vector4(1, 0, 0, 1.0f);

	Vector3 worldPos = modelMatrix.GetPositionVector();

	DrawLine(worldPos, worldPos + (right * scaleBoost)	, Debug::RED, time);
	DrawLine(worldPos, worldPos + (up * scaleBoost)		, Debug::GREEN, time);
	DrawLine(worldPos, worldPos + (fwd * scaleBoost)	, Debug::BLUE, time);
}


void Debug::FlushRenderables(float dt) {
	if (!renderer) {
		return;
	}
	for (const auto& i : stringEntries) {
		renderer->DrawString(i.data, i.position);
	}
	int trim = 0;
	for (int i = 0; i < lineEntries.size(); ) {
		DebugLineEntry* e = &lineEntries[i]; 
		renderer->DrawLine(e->start, e->end, e->color);
		e->time -= dt;
		if (e->time < 0) {			
			trim++;				
			lineEntries[i] = lineEntries[lineEntries.size() - trim];
		}
		else {
			++i;
		}		
		if (i + trim >= lineEntries.size()) {
			break;
		}
	}
	lineEntries.resize(lineEntries.size() - trim);

	stringEntries.clear();
}
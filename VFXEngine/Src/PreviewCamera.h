#pragma once
#include "Camera.h"

class PreviewCamera : public Camera {
public:
	using Camera::Camera;
	~PreviewCamera() override = default;

	// Updateをオーバーライドして独自の操作処理を記述
	void Update(float deltaTime) override;
};
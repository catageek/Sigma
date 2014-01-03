#pragma once

#include "IGLView.h"

struct GLSixDOFView : public Sigma::IGLView {

    SET_COMPONENT_TYPENAME("SIX_DOF_VIEW");

	GLSixDOFView(int entityID);

	const glm::mat4 GetViewMatrix();
	virtual void Move(float right, float up, float forward);
	virtual glm::vec3 Restrict(glm::vec3 rotation) { return rotation; }
};

#include "pch.h"
#include "EditorGridPass.h"

void EditorGridPass::Init(const InitDesc& desc)
{
	m_renderer = std::make_unique<EditorGridRenderer>();
	m_renderer->Init({ .device = desc.dev, .gpuResMgr = desc.gpuResMgr });
}

void EditorGridPass::Execute(const PassExecuteDesc& desc)
{
	auto renderDesc = desc.renderDesc;

	if (m_renderer) {
		m_renderer->Render({ desc.renderDesc });
	}
}
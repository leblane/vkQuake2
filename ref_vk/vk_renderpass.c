/*
Copyright (C) 2018-2019 Krzysztof Kondrak

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "vk_local.h"


VkResult QVk_CreateRenderpass(qvkrenderpass_t *renderpass)
{
	qboolean msaaEnabled = renderpass->sampleCount != VK_SAMPLE_COUNT_1_BIT;

	VkAttachmentDescription colorAttachmentDesc = {
		.format = vk_swapchain.format,
		.samples = renderpass->sampleCount,
		.loadOp = renderpass->colorLoadOp,
		// if MSAA is enabled, we don't need to preserve rendered texture data since it's kept by MSAA resolve attachment
		.storeOp = msaaEnabled ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	// treat this attachment as an interim color stage if MSAA is enabled
	if (msaaEnabled)
		colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	else
		colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachmentDesc = {
		.format = QVk_FindDepthFormat(),
		.samples = renderpass->sampleCount,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	// resolve color target used if MSAA is enabled
	VkAttachmentDescription colorAttachmentResolveMSAA = {
		.format = vk_swapchain.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference colorAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentReference depthAttachmentRef = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference colorAttachmentResolveMSAARef = {
		.attachment = 2,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpassDesc = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pDepthStencilAttachment = &depthAttachmentRef
	};
	if (msaaEnabled)
		subpassDesc.pResolveAttachments = &colorAttachmentResolveMSAARef;

	VkAttachmentDescription attachments[] = { colorAttachmentDesc, depthAttachmentDesc };
	VkAttachmentDescription attachmentsMSAA[] = { colorAttachmentDesc, depthAttachmentDesc, colorAttachmentResolveMSAA };
	VkRenderPassCreateInfo rpCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = msaaEnabled ? 3 : 2,
		.pAttachments = msaaEnabled ? attachmentsMSAA : attachments,
		.subpassCount = 1,
		.pSubpasses = &subpassDesc
	};

	// subpass depencency: wait for color stage
	VkSubpassDependency spDep = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
	};

	rpCreateInfo.dependencyCount = 1;
	rpCreateInfo.pDependencies = &spDep;

	return vkCreateRenderPass(vk_device.logical, &rpCreateInfo, NULL, &renderpass->rp);
}

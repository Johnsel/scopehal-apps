/***********************************************************************************************************************
*                                                                                                                      *
* libscopeprotocols                                                                                                    *
*                                                                                                                      *
* Copyright (c) 2012-2022 Andrew D. Zonenberg and contributors                                                         *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

#ifndef ComputePipeline_h
#define ComputePipeline_h

#include "AcceleratorBuffer.h"

/**
	@brief A ComputePipeline encapsulates a Vulkan compute pipeline and all necessary resources to use it

	For now, only pure compute is supported, and there is no support for accessing images or other non-SSBO
	data types.

	A ComputePipeline is typically owned by a filter instance.
 */
class ComputePipeline
{
public:
	ComputePipeline(const std::string& shaderPath, size_t numSSBOs, size_t pushConstantSize);
	virtual ~ComputePipeline();

	/**
		@brief Binds a buffer to a descriptor slot
	 */
	template<class T>
	void BindBuffer(size_t i, AcceleratorBuffer<T>& buf, bool outputOnly = false)
	{
		if(m_computePipeline == nullptr)
			DeferredInit();

		buf.PrepareForGpuAccess(outputOnly);

		m_bufferInfo[i] = buf.GetBufferInfo();
		m_writeDescriptors[i] =
			vk::WriteDescriptorSet(**m_descriptorSet, i, 0, vk::DescriptorType::eStorageBuffer, {}, m_bufferInfo[i]);
	}

	/**
		@brief Binds a buffer to a descriptor slot
	 */
	template<class T>
	void BindBufferNonblocking(size_t i, AcceleratorBuffer<T>& buf, vk::raii::CommandBuffer& cmdBuf, bool outputOnly = false)
	{
		if(m_computePipeline == nullptr)
			DeferredInit();

		buf.PrepareForGpuAccessNonblocking(outputOnly, cmdBuf);

		m_bufferInfo[i] = buf.GetBufferInfo();
		m_writeDescriptors[i] =
			vk::WriteDescriptorSet(**m_descriptorSet, i, 0, vk::DescriptorType::eStorageBuffer, {}, m_bufferInfo[i]);
	}

	/**
		@brief Helper function to insert a memory barrier in a command buffer
	 */
	void AddComputeMemoryBarrier(vk::raii::CommandBuffer& cmdBuf)
	{
		cmdBuf.pipelineBarrier(
			vk::PipelineStageFlagBits::eComputeShader,
			vk::PipelineStageFlagBits::eComputeShader,
			{},
			vk::MemoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead),
			{},
			{});
	}

	/**
		@brief Dispatches a compute operation to a command buffer
	 */
	template<class T>
	void Dispatch(vk::raii::CommandBuffer& cmdBuf, T pushConstants, uint32_t x, uint32_t y=1, uint32_t z=1)
	{
		if(m_computePipeline == nullptr)
			DeferredInit();

		g_vkComputeDevice->updateDescriptorSets(m_writeDescriptors, nullptr);
		cmdBuf.bindPipeline(vk::PipelineBindPoint::eCompute, **m_computePipeline);
		cmdBuf.pushConstants<T>(
			**m_pipelineLayout,
			vk::ShaderStageFlagBits::eCompute,
			0,
			pushConstants);
		cmdBuf.bindDescriptorSets(
			vk::PipelineBindPoint::eCompute,
			**m_pipelineLayout,
			0,
			**m_descriptorSet,
			{});
		cmdBuf.dispatch(x, y, z);
	}

protected:
	void DeferredInit();

	std::string m_shaderPath;
	size_t m_numSSBOs;
	size_t m_pushConstantSize;

	std::unique_ptr<vk::raii::ShaderModule> m_shaderModule;
	std::unique_ptr<vk::raii::Pipeline> m_computePipeline;
	std::unique_ptr<vk::raii::PipelineLayout> m_pipelineLayout;
	std::unique_ptr<vk::raii::DescriptorSetLayout> m_descriptorSetLayout;
	std::unique_ptr<vk::raii::DescriptorPool> m_descriptorPool;
	std::unique_ptr<vk::raii::DescriptorSet> m_descriptorSet;

	std::vector<vk::WriteDescriptorSet> m_writeDescriptors;
	std::vector<vk::DescriptorBufferInfo> m_bufferInfo;
};

#endif

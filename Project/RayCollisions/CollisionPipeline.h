#pragma once
#include "../SharedUtils/RenderingPipeline.h"

class CollisionPipeline : public ::RenderingPipeline, inherit_shared_from_this<::RenderingPipeline, CollisionPipeline>
{
public:
	using SharedPtr = std::shared_ptr<CollisionPipeline>;

	static SharedPtr create() { return SharedPtr(new CollisionPipeline()); }
	virtual ~CollisionPipeline() = default;

protected:
};
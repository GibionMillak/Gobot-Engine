func test():
	# Directly.
	var tree := SceneTree.new()
	tree.root = Window.new()
	tree.free()

	# Indirectly.
	var state := PhysicsIntersectRayResult3D.new()
	state.position.x += 1.0

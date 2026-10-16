func subtest_attribute(state):
	state.position.x -= 1.0
	print("end subtest_attribute")

func subtest_variable_index(state, prop):
	state[prop].x = 1.0
	print("end subtest_variable_index")

func test():
	var state = PhysicsIntersectRayResult3D.new()
	subtest_attribute(state)
	subtest_variable_index(state, &"position")

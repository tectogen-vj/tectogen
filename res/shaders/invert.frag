void invert(out vec3 col, in vec3 col1) {
	col=vec3(1.-col1.x, 1.-col1.y, 1.-col1.z);
}
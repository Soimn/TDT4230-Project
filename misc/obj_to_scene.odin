package obj_to_scene;

import "core:fmt";
import "core:os";
import "core:strings";
import "core:strconv";
import "core:slice";
import "core:math/linalg";

/*   0 - 3   *   4 - 7   *   8 - 11     *           *
 *--------------------------------------------------*
 * tri_count * mat_count *  light_count *           *
 *--------------------------------------------------*
 * triangle intersection data                       *
 * alpha:  p0.x  p0.y  p0.z  p2.x                   *
 * beta:   p1.x  p1.y  p1.z  p2.y                   *
 * gamma:  p2.z                                     *
 *--------------------------------------------------*
 * triangle material data                           *
 * n0n2x:  n0.x  n0.y  n0.z  n2.x                   *
 * n1b2y:  n1.x  n1.y  n1.z  n2.y                   *
 * n2zmat: n2.z  mat                                *
 *--------------------------------------------------*
 * bounding spheres                                 *
 * pr:     p.x   p.y   p.z   r                      *
 *--------------------------------------------------*
 * materials                                        *
 * color:  r     g     b     a                      *
 * kind:   kind                                     *
 *--------------------------------------------------*
 * lights                                           *
 * p0nx:   p0.x  p0.y  p0.z  n.x                    *
 * p1ny:   p1.x  p1.y  p1.z  n.y                    *
 * p2nz:   p2.x  p2.y  p2.z  n.z                    *
 * areaid: area  id                                 *
 *--------------------------------------------------*
 */

Triangle :: struct
{
	p0:  [3]f32,
	p1:  [3]f32,
	p2:  [3]f32,
	uv0: [2]f32,
	uv1: [2]f32,
	uv2: [2]f32,
	n0:  [3]f32,
	n1:  [3]f32,
	n2:  [3]f32,
	mat: f32,
}

Triangle_Data :: struct
{
	p0_x:   f32,
	p0_y:   f32,
	p0_z:   f32,
	p2_x:   f32,

	p1_x:   f32,
	p1_y:   f32,
	p1_z:   f32,
	p2_y:   f32,

	p2_z:   f32,
	_pad_0: f32,
	_pad_1: f32,
	_pad_2: f32,
}

Triangle_Material_Data :: struct
{
	n0_x:   f32,
	n0_y:   f32,
	n0_z:   f32,
	n2_x:   f32,

	n1_x:   f32,
	n1_y:   f32,
	n1_z:   f32,
	n2_y:   f32,

	n2_z:   f32,
	mat:    f32,
	_pad_0: f32,
	_pad_1: f32,
}

Bounding_Sphere :: struct
{
	p_x: f32,
	p_y: f32,
	p_z: f32,
	r:   f32,
}

Material_Kind :: enum u32
{
	Diffuse    = 0,
	Reflective = 1,
	Refractive = 2,
	Light      = 3,
}

Material :: struct
{
	color_r: f32,
	color_g: f32,
	color_b: f32,
	color_a: f32,
	kind:    Material_Kind,
	_pad_0: [3]u32,
}

Light :: struct
{
	p0nx: [4]f32,
	p1ny: [4]f32,
	p2nz: [4]f32,
	areaidmat: [4]f32,
}

Materials := [?]Material{
	0 = Material{
		color_r = 0.8,
		color_g = 0.8,
		color_b = 0.8,
		color_a = 0,
		kind    = Material_Kind.Diffuse,
	},
	1 = Material{
		color_r = 0.051991,
		color_g = 0.252292,
		color_b = 0.8,
		color_a = 0,
		kind    = Material_Kind.Diffuse,
	},
	2 = Material{
		color_r = 0.8,
		color_g = 0.152912,
		color_b = 0.072971,
		color_a = 0,
		kind    = Material_Kind.Diffuse,
	},
	3 = Material{
		color_r = 1,
		color_g = 1,
		color_b = 1,
		color_a = 100,
		kind    = Material_Kind.Light,
	},
	4 = Material{
		color_r = 1,
		color_g = 1,
		color_b = 1,
		color_a = 80,
		kind    = Material_Kind.Light,
	},
	5 = Material{
		color_r = 1,
		color_g = 1,
		color_b = 1,
		color_a = 1.2,
		kind    = Material_Kind.Refractive,
	},
	6 = Material{
		color_r = 0.55,
		color_g = 0,
		color_b = 0.55,
		color_a = 1.52,
		kind    = Material_Kind.Refractive,
	},
};

main :: proc()
{
	if len(os.args) != 2
	{
		fmt.println("ERROR: Invalid number of arguments. Correct usage: obj_to_scene <path-to-obj-file>");
		return;
	}

	obj_file: string;

	if obj_file_data, ok := os.read_entire_file_from_filename(os.args[1]); ok do obj_file = string(obj_file_data);
	else
	{
		fmt.println("ERROR: Failed to read specified obj file");
		return;
	}

	lines := strings.split(obj_file, "\n");
	assert(len(lines[len(lines)-1]) == 0);
	lines = lines[:len(lines)-1];
	
	vertices  := make([dynamic][3]f32);
	uvs       := make([dynamic][2]f32);
	normals   := make([dynamic][3]f32);
	triangles := make([dynamic]Triangle);
	lights    := make([dynamic]Light);

	assert(len(lines) > 4);
	assert(lines[0][0] == '#');
	assert(lines[1][0] == '#');
	assert(strings.has_prefix(lines[2], "mtllib"));
	cursor := 3;

	for cursor < len(lines)
	{
		assert(strings.has_prefix(lines[cursor], "o "));
		cursor += 1;

		for ; strings.has_prefix(lines[cursor], "v "); cursor += 1
		{
			fields := strings.fields(lines[cursor]);

			assert(len(fields) == 4);
			x, x_ok := strconv.parse_f32(fields[1]);
			y, y_ok := strconv.parse_f32(fields[2]);
			z, z_ok := strconv.parse_f32(fields[3]);
			assert(x_ok && y_ok && z_ok);

			append(&vertices, [3]f32{x,y,z});
		}

		for ; strings.has_prefix(lines[cursor], "vt "); cursor += 1
		{
			fields := strings.fields(lines[cursor]);

			assert(len(fields) == 3);
			x, x_ok := strconv.parse_f32(fields[1]);
			y, y_ok := strconv.parse_f32(fields[2]);
			assert(x_ok && y_ok);

			append(&uvs, [2]f32{x,y});
		}

		for ; strings.has_prefix(lines[cursor], "vn "); cursor += 1
		{
			fields := strings.fields(lines[cursor]);

			assert(len(fields) == 4);
			x, x_ok := strconv.parse_f32(fields[1]);
			y, y_ok := strconv.parse_f32(fields[2]);
			z, z_ok := strconv.parse_f32(fields[3]);
			assert(x_ok && y_ok && z_ok);

			append(&normals, [3]f32{x,y,z});
		}

		assert(strings.has_prefix(lines[cursor], "usemtl mat"));
		material_id, mat_ok := strconv.parse_int(strings.trim_prefix(lines[cursor], "usemtl mat"));
		assert(mat_ok);
		assert(material_id >= 0 && material_id < len(Materials));
		cursor += 1;

		assert(strings.has_prefix(lines[cursor], "s "));
		cursor += 1;

		for ; cursor < len(lines) && strings.has_prefix(lines[cursor], "f "); cursor += 1
		{
			tri := Triangle{};

			fields := strings.fields(lines[cursor]);

			assert(len(fields) == 4);
			param0 := strings.split(fields[1], "/");
			param1 := strings.split(fields[2], "/");
			param2 := strings.split(fields[3], "/");
			assert(len(param0) == 3 && len(param1) == 3 && len(param2) == 3);

			parse_param :: proc(param: []string, p: ^[3]f32, uv: ^[2]f32, n: ^[3]f32, vertices: [][3]f32, uvs: [][2]f32, normals: [][3]f32)
			{
				p_index,  p_ok  := strconv.parse_int(param[0], 10);
				uv_index, uv_ok := strconv.parse_int(param[1], 10);
				n_index,  n_ok  := strconv.parse_int(param[2], 10);
				p_index  -= 1;
				uv_index -= 1;
				n_index  -= 1;
				assert(p_ok && uv_ok && n_ok);
				assert(p_index >= 0 && uv_index >= 0 && n_index >= 0);

				p^  = vertices[p_index];
				uv^ = uvs[uv_index];
				n^  = normals[n_index];
			}

			parse_param(param0, &tri.p0, &tri.uv0, &tri.n0, vertices[:], uvs[:], normals[:]);
			parse_param(param1, &tri.p1, &tri.uv1, &tri.n1, vertices[:], uvs[:], normals[:]);
			parse_param(param2, &tri.p2, &tri.uv2, &tri.n2, vertices[:], uvs[:], normals[:]);

			tri.mat = f32(material_id);

			append(&triangles, tri);
		}
	}

	triangle_data          := make([dynamic]Triangle_Data);
	triangle_material_data := make([dynamic]Triangle_Material_Data);
	bounding_spheres       := make([dynamic]Bounding_Sphere);

	for tri in triangles
	{
		tri_data := Triangle_Data{
			p0_x = tri.p0.x,
			p0_y = tri.p0.y,
			p0_z = tri.p0.z,
			p2_x = tri.p2.x,

			p1_x = tri.p1.x,
			p1_y = tri.p1.y,
			p1_z = tri.p1.z,
			p2_y = tri.p2.y,

			p2_z = tri.p2.z,
		};

		tri_mat := Triangle_Material_Data{
			n0_x  = tri.n0.x,
			n0_y  = tri.n0.y,
			n0_z  = tri.n0.z,
			n2_x  = tri.n2.x,

			n1_x  = tri.n1.x,
			n1_y  = tri.n1.y,
			n1_z  = tri.n1.z,
			n2_y  = tri.n2.y,

			n2_z  = tri.n2.z,
			mat   = tri.mat,
		};

		a := tri.p0;
		b := tri.p1;
		c := tri.p2;
		m := (a + b + c)/3;

		sphere := Bounding_Sphere{
			p_x = m.x,
			p_y = m.y,
			p_z = m.z,
			r   = max(linalg.vector_length(a-m), max(linalg.vector_length(b-m), linalg.vector_length(c-m))),
		};

		if Materials[int(tri.mat)].kind == .Light
		{
			id := len(triangle_data);

			e1 := tri.p1 - tri.p0;
			e2 := tri.p2 - tri.p0;

			scaled_normal := linalg.vector_cross3(e1, e2);
			area   := linalg.vector_length(scaled_normal);
			normal := linalg.vector_normalize(scaled_normal);

			light := Light{
				p0nx      = [4]f32{tri.p0.x, tri.p0.y, tri.p0.z, normal.x},
				p1ny      = [4]f32{tri.p1.x, tri.p1.y, tri.p1.z, normal.y},
				p2nz      = [4]f32{tri.p2.x, tri.p2.y, tri.p2.z, normal.z},
				areaidmat = [4]f32{area, f32(id), tri.mat, 0},
			};

			append(&lights, light);
		}

		append(&triangle_data, tri_data);
		append(&triangle_material_data, tri_mat);
		append(&bounding_spheres, sphere);
	}

	obj_file_info, _ := os.lstat(os.args[1]);
	filename := strings.concatenate([]string{strings.trim_suffix(obj_file_info.name, ".obj"), ".scene"});

	scene_builder := strings.builder_make_none();

	tri_count   := transmute([4]u8) u32(len(triangles));
	mat_count   := transmute([4]u8) u32(len(Materials));
	light_count := transmute([4]u8) u32(len(lights));
	strings.write_bytes(&scene_builder, tri_count[:]);
	strings.write_bytes(&scene_builder, mat_count[:]);
	strings.write_bytes(&scene_builder, light_count[:]);

	strings.write_bytes(&scene_builder, slice.to_bytes(triangle_data[:]));
	strings.write_bytes(&scene_builder, slice.to_bytes(triangle_material_data[:]));
	strings.write_bytes(&scene_builder, slice.to_bytes(bounding_spheres[:]));
	strings.write_bytes(&scene_builder, slice.to_bytes(Materials[:]));
	strings.write_bytes(&scene_builder, slice.to_bytes(lights[:]));

	strings.write_byte(&scene_builder, 0);

	scene_data := transmute([]u8) strings.to_string(scene_builder);
	os.remove(filename);
	succeeded_write := os.write_entire_file(filename, scene_data, false);
	assert(succeeded_write);
}

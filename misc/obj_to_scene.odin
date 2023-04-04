package obj_to_scene;

import "core:fmt";
import "core:os";
import "core:strings";
import "core:strconv";
import "core:slice";
import "core:math/linalg";

/*   0 - 4   *                                      *
 *--------------------------------------------------*
 * tri_count *                                      *
 *--------------------------------------------------*
 * triangle intersection data                       *
 * alpha:  p0.x  p0.y  p0.z  p2.x                   *
 * beta:   p1.x  p1.y  p1.z  p2.y                   *
 * gamma:  p2.z                                     *
 *--------------------------------------------------*
 * triangle material data                           *
 * n0uv0x: n0.x  n0.y  n0.z  uv0.x                  *
 * n1uv0y: n1.x  n1.y  n1.z  uv0.y                  *
 * n2:     n2.x  n2.y  n2.z                         *
 * uv12:   uv1.x uv1.y uv2.x uv2.y                  *
 *--------------------------------------------------*
 * bounding spheres                                 *
 * pr:     p.x   p.y   p.z   r                      *
 *--------------------------------------------------*
 */

Triangle :: struct
{
	p0: [3]f32,
	p1: [3]f32,
	p2: [3]f32,
	uv0: [2]f32,
	uv1: [2]f32,
	uv2: [2]f32,
	n0: [3]f32,
	n1: [3]f32,
	n2: [3]f32,
}

Triangle_Data :: struct
{
	p0_x: f32,
	p0_y: f32,
	p0_z: f32,
	p2_x: f32,

	p1_x: f32,
	p1_y: f32,
	p1_z: f32,
	p2_y: f32,

	p2_z: f32,
	_pad_0: f32,
	_pad_1: f32,
	_pad_2: f32,
}

Triangle_Material_Data :: struct
{
	n0_x: f32,
	n0_y: f32,
	n0_z: f32,
	uv0_x: f32,

	n1_x: f32,
	n1_y: f32,
	n1_z: f32,
	uv0_y: f32,

	n2_x: f32,
	n2_y: f32,
	n2_z: f32,
	_pad_0: f32,

	uv1_x: f32,
	uv1_y: f32,
	uv2_x: f32,
	uv2_y: f32,
}

Bounding_Sphere :: struct
{
	p_x: f32,
	p_y: f32,
	p_z: f32,
	r:   f32,
}

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
	
	vertices := make([dynamic][3]f32);
	uvs      := make([dynamic][2]f32);
	normals  := make([dynamic][3]f32);
	triangles := make([dynamic]Triangle);

	for line in lines
	{
		tline := strings.trim_left_space(line);
		if len(tline) == 0 || tline[0] == '#' do continue

		fields := strings.fields(tline);

		if fields[0] == "v"
		{
			x, x_ok := strconv.parse_f32(fields[1]);
			y, y_ok := strconv.parse_f32(fields[2]);
			z, z_ok := strconv.parse_f32(fields[3]);
			assert(x_ok && y_ok && z_ok && len(fields) == 4);

			append(&vertices, [3]f32{x,y,z});
		}
		else if fields[0] == "vt"
		{
			x, x_ok := strconv.parse_f32(fields[1]);
			y, y_ok := strconv.parse_f32(fields[2]);
			assert(x_ok && y_ok && len(fields) == 3);

			append(&uvs, [2]f32{x,y});
		}
		else if fields[0] == "vn"
		{
			x, x_ok := strconv.parse_f32(fields[1]);
			y, y_ok := strconv.parse_f32(fields[2]);
			z, z_ok := strconv.parse_f32(fields[3]);
			assert(x_ok && y_ok && z_ok && len(fields) == 4);

			append(&normals, [3]f32{x,y,z});
		}
		else if fields[0] == "f"
		{
			tri := Triangle{};

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

			append(&triangles, tri);
		}
		else if fields[0] == "usemtl"
		{
			if fields[1] == "light_source"
			{
			}
		}
		else
		{
			fmt.println("Ignored:", tline);
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
			uv0_x = tri.uv0.x,

			n1_x  = tri.n1.x,
			n1_y  = tri.n1.y,
			n1_z  = tri.n1.z,
			uv0_y = tri.uv0.y,

			n2_x  = tri.n2.x,
			n2_y  = tri.n2.y,
			n2_z  = tri.n2.z,

			uv1_x = tri.uv1.x,
			uv1_y = tri.uv1.y,
			uv2_x = tri.uv2.x,
			uv2_y = tri.uv2.y,
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

		append(&triangle_data, tri_data);
		append(&triangle_material_data, tri_mat);
		append(&bounding_spheres, sphere);
	}

	obj_file_info, _ := os.lstat(os.args[1]);
	filename := strings.concatenate([]string{strings.trim_suffix(obj_file_info.name, ".obj"), ".scene"});

	scene_builder := strings.builder_make_none();

	tri_count := transmute([4]u8) u32(len(triangles));
	strings.write_bytes(&scene_builder, tri_count[:]);

	strings.write_bytes(&scene_builder, slice.to_bytes(triangle_data[:]));
	strings.write_bytes(&scene_builder, slice.to_bytes(triangle_material_data[:]));
	strings.write_bytes(&scene_builder, slice.to_bytes(bounding_spheres[:]));

	strings.write_byte(&scene_builder, 0);

	scene_data := transmute([]u8) strings.to_string(scene_builder);
	os.remove(filename);
	succeeded_write := os.write_entire_file(filename, scene_data, false);
	assert(succeeded_write);
}

#include "rasterizer_renderer.h"

#include "utils/resource_utils.h"


void cg::renderer::rasterization_renderer::init()
{

	rasterizer = std::make_shared<cg::renderer::rasterizer<cg::vertex, cg::unsigned_color>>();
	render_target = std::make_shared<cg::resource<cg::unsigned_color>>(settings->width, settings->height);
	rasterizer->set_viewport(settings->width, settings->height);
	rasterizer->set_render_target(render_target);

	model = std::make_shared<cg::world::model>();
	model->load_obj(settings->model_path);

	camera = std::make_shared<cg::world::camera>();
	camera->set_height(static_cast<float>(settings->height));


	camera->set_width(static_cast<float>(settings->width));
	camera->set_position(float3{
			settings->camera_position[0],
			settings->camera_position[1],
			settings->camera_position[2],
	});

	camera->set_theta(settings->camera_theta);
	camera->set_phi(settings->camera_phi);
	camera->set_angle_of_view(settings->camera_angle_of_view);
	camera->set_z_near(settings->camera_z_near);
	camera->set_z_far(settings->camera_z_far);

	depth_buffer = std::make_shared<cg::resource<float>>(settings->width, settings->height);
	rasterizer->set_render_target(render_target, depth_buffer);
}

void cg::renderer::rasterization_renderer::render()
{

	float4x4 matrix = mul(
			camera->get_projection_matrix(),
			camera->get_view_matrix(),
			model->get_world_matrix());

	rasterizer->vertex_shader = [&](float4 vertex, cg::vertex data) {
		auto processed = mul(matrix, vertex);
		return std::make_pair(processed, data);
	};


	rasterizer->pixel_shader = [](cg::vertex data, float z) {
		return cg::color{
				data.ambient_r,
				data.ambient_g,
				data.ambient_b,
		};
	};


	{
		const auto begin = std::chrono::high_resolution_clock::now();
		backround_r = 111;
		backround_g = 5;
		backround_b = 243;

		rasterizer->clear_render_target({backround_r, backround_g, backround_b});
		const auto end = std::chrono::high_resolution_clock::now();

		std::chrono::duration<float, std::milli> duration = end - begin;

		std::cout << "Clearing job was executed for " << duration.count() << " ms" << std::endl;
	}

	{
		const auto begin= std::chrono::high_resolution_clock::now();
		for (unsigned int s_id = 0; s_id < model->get_index_buffers().size(); s_id++) {

			rasterizer->set_index_buffer(model->get_index_buffers()[s_id]);
			rasterizer->set_vertex_buffer(model->get_vertex_buffers()[s_id]);

			rasterizer->draw(
					model->get_index_buffers()[s_id]->get_number_of_elements(),
					0);
		}
		const auto finish = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float, std::milli> rendering_duration = finish - begin;
		std::cout << "Rendering took " << rendering_duration.count() << "ms"<<std::endl;
	}

	auto imageCopy = *render_target;
	rasterizer->clear_render_target({backround_r, backround_g, backround_b});

	for(int i = 0; i < render_target->get_number_of_elements(); ++i) {
		render_target->item(i).r = imageCopy.item(i).r * settings->transparency + render_target->item(i).r * (1 - settings->transparency);
		render_target->item(i).g = imageCopy.item(i).g * settings->transparency + render_target->item(i).g * (1 - settings->transparency);
		render_target->item(i).b = imageCopy.item(i).b * settings->transparency + render_target->item(i).b * (1 - settings->transparency);
	}


	utils::save_resource(*render_target, settings->result_path);
}

void cg::renderer::rasterization_renderer::destroy() {}

void cg::renderer::rasterization_renderer::update() {}
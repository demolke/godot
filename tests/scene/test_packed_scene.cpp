/**************************************************************************/
/*  test_packed_scene.cpp                                                 */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "tests/test_macros.h"

TEST_FORCE_LINK(test_packed_scene)

#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/callable_mp.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "scene/resources/packed_scene.h"

#include "tests/test_utils.h"

namespace TestPackedScene {

TEST_CASE("[PackedScene] Pack Scene and Retrieve State") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	PackedScene packed_scene;
	const Error err = packed_scene.pack(scene);
	CHECK(err == OK);

	// Retrieve the packed state.
	Ref<SceneState> state = packed_scene.get_state();
	CHECK(state.is_valid());
	CHECK(state->get_node_count() == 1);
	CHECK(state->get_node_name(0) == "TestScene");

	memdelete(scene);
}

TEST_CASE("[PackedScene] Signals Preserved when Packing Scene") {
	// Create main scene
	// root
	// `- sub_node (local)
	// `- sub_scene (instance of another scene)
	//    `- sub_scene_node (owned by sub_scene)
	Node *main_scene_root = memnew(Node);
	Node *sub_node = memnew(Node);
	Node *sub_scene_root = memnew(Node);
	Node *sub_scene_node = memnew(Node);

	main_scene_root->add_child(sub_node);
	sub_node->set_owner(main_scene_root);

	sub_scene_root->add_child(sub_scene_node);
	sub_scene_node->set_owner(sub_scene_root);

	main_scene_root->add_child(sub_scene_root);
	sub_scene_root->set_owner(main_scene_root);

	SUBCASE("Signals that should be saved") {
		int main_flags = Object::CONNECT_PERSIST;
		// sub node to a node in main scene
		sub_node->connect("ready", callable_mp(main_scene_root, &Node::is_ready), main_flags);
		// subscene root to a node in main scene
		sub_scene_root->connect("ready", callable_mp(main_scene_root, &Node::is_ready), main_flags);
		//subscene root to subscene root (connected within main scene)
		sub_scene_root->connect("ready", callable_mp(sub_scene_root, &Node::is_ready), main_flags);

		// Pack the scene.
		Ref<PackedScene> packed_scene;
		packed_scene.instantiate();
		const Error err = packed_scene->pack(main_scene_root);
		CHECK(err == OK);

		// Make sure the right connections are in packed scene.
		Ref<SceneState> state = packed_scene->get_state();
		CHECK_EQ(state->get_connection_count(), 3);
	}

	/*
	// FIXME: This subcase requires GH-48064 to be fixed.
	SUBCASE("Signals that should not be saved") {
		int subscene_flags = Object::CONNECT_PERSIST | Object::CONNECT_INHERITED;
		// subscene node to itself
		sub_scene_node->connect("ready", callable_mp(sub_scene_node, &Node::is_ready), subscene_flags);
		// subscene node to subscene root
		sub_scene_node->connect("ready", callable_mp(sub_scene_root, &Node::is_ready), subscene_flags);
		//subscene root to subscene root (connected within sub scene)
		sub_scene_root->connect("ready", callable_mp(sub_scene_root, &Node::is_ready), subscene_flags);

		// Pack the scene.
		Ref<PackedScene> packed_scene;
		packed_scene.instantiate();
		const Error err = packed_scene->pack(main_scene_root);
		CHECK(err == OK);

		// Make sure the right connections are in packed scene.
		Ref<SceneState> state = packed_scene->get_state();
		CHECK_EQ(state->get_connection_count(), 0);
	}
	*/

	memdelete(main_scene_root);
}

TEST_CASE("[PackedScene] Clear Packed Scene") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	PackedScene packed_scene;
	packed_scene.pack(scene);

	// Clear the packed scene.
	packed_scene.clear();

	// Check if it has been cleared.
	Ref<SceneState> state = packed_scene.get_state();
	CHECK_FALSE(state->get_node_count() == 1);

	memdelete(scene);
}

TEST_CASE("[PackedScene] Can Instantiate Packed Scene") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	PackedScene packed_scene;
	packed_scene.pack(scene);

	// Check if the packed scene can be instantiated.
	const bool can_instantiate = packed_scene.can_instantiate();
	CHECK(can_instantiate == true);

	memdelete(scene);
}

TEST_CASE("[PackedScene] Instantiate Packed Scene") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	PackedScene packed_scene;
	packed_scene.pack(scene);

	// Instantiate the packed scene.
	Node *instance = packed_scene.instantiate();
	CHECK(instance != nullptr);
	CHECK(instance->get_name() == "TestScene");

	memdelete(scene);
	memdelete(instance);
}

TEST_CASE("[PackedScene] Instantiate Packed Scene With Children") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Add persisting child nodes to the scene.
	Node *child1 = memnew(Node);
	child1->set_name("Child1");
	scene->add_child(child1);
	child1->set_owner(scene);

	Node *child2 = memnew(Node);
	child2->set_name("Child2");
	scene->add_child(child2);
	child2->set_owner(scene);

	// Add non persisting child node to the scene.
	Node *child3 = memnew(Node);
	child3->set_name("Child3");
	scene->add_child(child3);

	// Pack the scene.
	PackedScene packed_scene;
	packed_scene.pack(scene);

	// Instantiate the packed scene.
	Node *instance = packed_scene.instantiate();
	CHECK(instance != nullptr);
	CHECK(instance->get_name() == "TestScene");

	// Validate the child nodes of the instantiated scene.
	CHECK(instance->get_child_count() == 2);
	CHECK(instance->get_child(0)->get_name() == "Child1");
	CHECK(instance->get_child(1)->get_name() == "Child2");
	CHECK(instance->get_child(0)->get_owner() == instance);
	CHECK(instance->get_child(1)->get_owner() == instance);

	memdelete(scene);
	memdelete(instance);
}

TEST_CASE("[PackedScene] Set Path") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	PackedScene packed_scene;
	packed_scene.pack(scene);

	// Set a new path for the packed scene.
	const String new_path = "NewTestPath";
	packed_scene.set_path(new_path);

	// Check if the path has been set correctly.
	Ref<SceneState> state = packed_scene.get_state();
	CHECK(state.is_valid());
	CHECK(state->get_path() == new_path);

	memdelete(scene);
}

TEST_CASE("[PackedScene] Replace State") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	PackedScene packed_scene;
	packed_scene.pack(scene);

	// Create another scene state to replace with.
	Ref<SceneState> new_state = memnew(SceneState);
	new_state->set_path("NewPath");

	// Replace the state.
	packed_scene.replace_state(new_state);

	// Check if the state has been replaced.
	Ref<SceneState> state = packed_scene.get_state();
	CHECK(state.is_valid());
	CHECK(state == new_state);

	memdelete(scene);
}

TEST_CASE("[PackedScene] Recreate State") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	Ref<PackedScene> packed_scene;
	packed_scene.instantiate();
	packed_scene->pack(scene);

	// Recreate the state.
	packed_scene->recreate_state();

	// Check if the state has been recreated.
	Ref<SceneState> state = packed_scene->get_state();
	CHECK(state.is_valid());
	CHECK(state->get_node_count() == 0); // Since the state was recreated, it should be empty.

	memdelete(scene);
}

TEST_CASE("[PackedScene] Sub-tree reference via node= (text format)") {
	// Source scene: Source -> Branch -> Leaf.
	Node3D *source_root = memnew(Node3D);
	source_root->set_name("Source");
	Node3D *branch = memnew(Node3D);
	branch->set_name("Branch");
	Node3D *leaf = memnew(Node3D);
	leaf->set_name("Leaf");
	source_root->add_child(branch);
	branch->set_owner(source_root);
	branch->add_child(leaf);
	leaf->set_owner(source_root);

	Ref<PackedScene> source_packed;
	source_packed.instantiate();
	REQUIRE(source_packed->pack(source_root) == OK);

	const String source_path = TestUtils::get_temp_path("subtree_source.tscn");
	REQUIRE(ResourceSaver::save(source_packed, source_path) == OK);

	Ref<PackedScene> source_loaded = ResourceLoader::load(source_path, "PackedScene", ResourceFormatLoader::CACHE_MODE_REPLACE);
	REQUIRE(source_loaded.is_valid());

	SUBCASE("get_subtree_state re-roots at the requested node") {
		Ref<SceneState> sub = source_loaded->get_state()->get_subtree_state(NodePath("Branch"));
		REQUIRE(sub.is_valid());
		REQUIRE(sub->get_node_count() == 2);
		CHECK(String(sub->get_node_name(0)) == "Branch");
		CHECK(String(sub->get_node_path(0)) == ".");
		CHECK(String(sub->get_node_name(1)) == "Leaf");
	}

	const String synthetic = source_path + ResourceFormatLoaderSubScene::SUB_SCENE_SEPARATOR + "Branch";

	SUBCASE("Synthetic @node= path loads a re-rooted PackedScene") {
		Ref<PackedScene> wrapper = ResourceLoader::load(synthetic, "PackedScene");
		REQUIRE(wrapper.is_valid());
		Node *inst = wrapper->instantiate();
		REQUIRE(inst != nullptr);
		CHECK(inst->get_name() == StringName("Branch"));
		REQUIRE(inst->get_child_count() == 1);
		CHECK(inst->get_child(0)->get_name() == StringName("Leaf"));
		memdelete(inst);
	}

	// Host scene instances the sub-tree, then is saved.
	Ref<PackedScene> wrapper = ResourceLoader::load(synthetic, "PackedScene");
	REQUIRE(wrapper.is_valid());
	Node *branch_instance = wrapper->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE);
	REQUIRE(branch_instance != nullptr);

	Node3D *host_root = memnew(Node3D);
	host_root->set_name("Host");
	host_root->add_child(branch_instance);
	branch_instance->set_owner(host_root);

	Ref<PackedScene> host_packed;
	host_packed.instantiate();
	REQUIRE(host_packed->pack(host_root) == OK);

	const String host_path = TestUtils::get_temp_path("subtree_host.tscn");
	REQUIRE(ResourceSaver::save(host_packed, host_path) == OK);

	SUBCASE("Saver writes a node= PackedScene ext_resource") {
		Ref<FileAccess> f = FileAccess::open(host_path, FileAccess::READ);
		REQUIRE(f.is_valid());
		const String text = f->get_as_text();
		CHECK(text.contains("[ext_resource"));
		CHECK(text.contains("type=\"PackedScene\""));
		CHECK(text.contains("node=\"Branch\""));
		CHECK(text.contains(vformat("path=\"%s\"", source_path)));
		// The synthetic "@node=" path must never be written verbatim.
		CHECK_FALSE(text.contains("@node="));
	}

	SUBCASE("Loading the host re-instantiates the sub-tree") {
		Ref<PackedScene> host_loaded = ResourceLoader::load(host_path, "PackedScene", ResourceFormatLoader::CACHE_MODE_REPLACE);
		REQUIRE(host_loaded.is_valid());
		Node *inst = host_loaded->instantiate();
		REQUIRE(inst != nullptr);
		Node *b = inst->get_node_or_null(NodePath("Branch"));
		REQUIRE(b != nullptr);
		REQUIRE(b->get_child_count() == 1);
		CHECK(b->get_node_or_null(NodePath("Leaf")) != nullptr);
		memdelete(inst);
	}

	memdelete(source_root);
	memdelete(host_root);
}

TEST_CASE("[PackedScene] Sub-tree reference survives text<->binary conversion") {
	// Source scene: Source -> Branch -> Leaf, saved as text.
	Node3D *source_root = memnew(Node3D);
	source_root->set_name("Source");
	Node3D *branch = memnew(Node3D);
	branch->set_name("Branch");
	Node3D *leaf = memnew(Node3D);
	leaf->set_name("Leaf");
	source_root->add_child(branch);
	branch->set_owner(source_root);
	branch->add_child(leaf);
	leaf->set_owner(source_root);

	Ref<PackedScene> source_packed;
	source_packed.instantiate();
	REQUIRE(source_packed->pack(source_root) == OK);

	const String source_path = TestUtils::get_temp_path("xfmt_source.tscn");
	REQUIRE(ResourceSaver::save(source_packed, source_path) == OK);

	// Build a host that instances the Branch sub-tree and save it as TEXT.
	const String synthetic = source_path + ResourceFormatLoaderSubScene::SUB_SCENE_SEPARATOR + "Branch";
	Ref<PackedScene> wrapper = ResourceLoader::load(synthetic, "PackedScene");
	REQUIRE(wrapper.is_valid());
	Node *branch_instance = wrapper->instantiate(PackedScene::GEN_EDIT_STATE_INSTANCE);
	REQUIRE(branch_instance != nullptr);

	Node3D *host_root = memnew(Node3D);
	host_root->set_name("Host");
	host_root->add_child(branch_instance);
	branch_instance->set_owner(host_root);

	Ref<PackedScene> host_packed;
	host_packed.instantiate();
	REQUIRE(host_packed->pack(host_root) == OK);

	const String host_tscn = TestUtils::get_temp_path("xfmt_host.tscn");
	REQUIRE(ResourceSaver::save(host_packed, host_tscn) == OK);

	SUBCASE("text host -> binary -> text round-trips the node= reference") {
		// Load the text host and re-save it as a BINARY .scn.
		Ref<PackedScene> from_tscn = ResourceLoader::load(host_tscn, "PackedScene", ResourceFormatLoader::CACHE_MODE_REPLACE);
		REQUIRE(from_tscn.is_valid());
		const String host_scn = TestUtils::get_temp_path("xfmt_host.scn");
		REQUIRE(ResourceSaver::save(from_tscn, host_scn) == OK);

		// The binary scene must still instantiate the sub-tree.
		Ref<PackedScene> from_scn = ResourceLoader::load(host_scn, "PackedScene", ResourceFormatLoader::CACHE_MODE_REPLACE);
		REQUIRE(from_scn.is_valid());
		Node *inst = from_scn->instantiate();
		REQUIRE(inst != nullptr);
		Node *b = inst->get_node_or_null(NodePath("Branch"));
		REQUIRE(b != nullptr);
		REQUIRE(b->get_child_count() == 1);
		CHECK(b->get_node_or_null(NodePath("Leaf")) != nullptr);
		memdelete(inst);

		// Convert the binary scene back to TEXT and confirm the clean node= form
		// is written (and the synthetic "@node=" path never leaks to disk).
		const String host_tscn2 = TestUtils::get_temp_path("xfmt_host2.tscn");
		REQUIRE(ResourceSaver::save(from_scn, host_tscn2) == OK);
		Ref<FileAccess> f = FileAccess::open(host_tscn2, FileAccess::READ);
		REQUIRE(f.is_valid());
		const String text = f->get_as_text();
		CHECK(text.contains("node=\"Branch\""));
		CHECK(text.contains(vformat("path=\"%s\"", source_path)));
		CHECK_FALSE(text.contains("@node="));
	}

	SUBCASE("binary host reports the source scene (not the synthetic path) as a dependency") {
		// In the binary format the node locator is stored separately from the
		// source path, so dependency tracking sees the bare source scene. This is
		// what lets the source be relocated/renamed by path or UID without the
		// "@node=" locator corrupting reconstruction.
		Ref<PackedScene> from_tscn = ResourceLoader::load(host_tscn, "PackedScene", ResourceFormatLoader::CACHE_MODE_REPLACE);
		REQUIRE(from_tscn.is_valid());
		const String host_scn_dep = TestUtils::get_temp_path("xfmt_host_dep.scn");
		REQUIRE(ResourceSaver::save(from_tscn, host_scn_dep) == OK);

		List<String> deps;
		ResourceLoader::get_dependencies(host_scn_dep, &deps);
		bool found_source = false;
		for (const String &dep : deps) {
			// The dependency is reported as "type::path" (and never carries the
			// synthetic "@node=" locator).
			CHECK_FALSE(dep.contains("@node="));
			if (dep.contains(source_path)) {
				found_source = true;
			}
		}
		CHECK(found_source);
	}

	memdelete(source_root);
	memdelete(host_root);
}

} // namespace TestPackedScene

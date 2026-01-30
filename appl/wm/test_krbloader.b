implement TestKrbloader;

include "sys.m";
	sys: Sys;
include "draw.m";
include "krbloader.m";
	krbloader: Krbloader;

TestKrbloader: module
{
	init: fn(nil: ref Draw->Context, argv: list of string);
};

init(nil: ref Draw->Context, argv: list of string)
{
	sys = load Sys Sys->PATH;
	krbloader = load Krbloader "/dis/wm/krbloader.dis";
	if (krbloader == nil) {
		sys->fprint(sys->fildes(2), "test: cannot load krbloader: %r\n");
		raise "fail:load";
	}
	krbloader->init();

	# Get filename from args
	if (argv == nil || tl argv == nil) {
		sys->print("Usage: test_krbloader <file.krb>\n");
		raise "fail:usage";
	}
	filename := hd tl argv;

	sys->print("Testing KRB Loader with: %s\n", filename);
	sys->print("========================================\n");

	# Test 1: Load KRB file
	sys->print("\n[1] Loading KRB file...\n");
	krbfile := krbloader->krbload(filename);
	if (krbfile == nil) {
		sys->print("FAILED: %s\n", krbloader->get_error());
		raise "fail:load";
	}
	sys->print("SUCCESS: File loaded (%d bytes)\n", krbfile.size);

	# Test 2: Display header info
	sys->print("\n[2] Header Information:\n");
	hdr := krbfile.header;
	sys->print("  Magic: 0x%x (expected 0x4B52594E)\n", hdr.magic);
	sys->print("  Version: %d.%d\n", hdr.version_major, hdr.version_minor);
	sys->print("  Widget instances: %d\n", hdr.widget_instance_count);
	sys->print("  Properties: %d\n", hdr.property_count);
	sys->print("  Events: %d\n", hdr.event_count);

	# Test 3: String table
	sys->print("\n[3] String Table:\n");
	if (krbfile.strings != nil) {
		sys->print("  Strings loaded: %d\n", len krbfile.strings);
		for (i := 0; i < len krbfile.strings && i < 5; i++)
			sys->print("    [%d]: \"%s\"\n", i, krbfile.strings[i]);
		if (len krbfile.strings > 5)
			sys->print("    ... and %d more\n", len krbfile.strings - 5);
	}

	# Test 4: Build widget tree
	sys->print("\n[4] Building widget tree...\n");
	tree := krbloader->build_widget_tree(krbfile);
	if (tree == nil) {
		sys->print("FAILED: Could not build widget tree\n");
		raise "fail:tree";
	}
	sys->print("SUCCESS: Widget tree built\n");

	# Test 5: Display root widget
	if (tree.root != nil) {
		w := tree.root;
		sys->print("  Root widget:\n");
		sys->print("    ID: %s\n", w.id);
		sys->print("    Type: %s (0x%x)\n",
			krbloader->widget_type_name(w.type_id), w.type_id);
		sys->print("    Size: %dx%d\n", w.width, w.height);

		# Count children
		nchildren := 0;
		for (children := w.children; children != nil; children = tl children)
			nchildren++;
		sys->print("    Children: %d\n", nchildren);
	}

	# Test 6: Calculate layout
	sys->print("\n[5] Calculating layout (800x600)...\n");
	err := krbloader->calculate_layout(tree, 800, 600);
	if (err != 0) {
		sys->print("FAILED: Layout calculation error\n");
		raise "fail:layout";
	}
	sys->print("SUCCESS: Layout calculated\n");
	if (tree.root != nil) {
		w := tree.root;
		sys->print("  Root widget layout:\n");
		sys->print("    Position: (%d, %d)\n", w.x, w.y);
		sys->print("    Size: %dx%d\n", w.width, w.height);
	}

	# Test 7: List all widgets
	sys->print("\n[6] Widget Tree Summary:\n");
	count := 0;
	for (widgets := tree.widgets; widgets != nil; widgets = tl widgets)
		count++;
	sys->print("  Total widgets: %d\n", count);

	sys->print("\n========================================\n");
	sys->print("✓ ALL TESTS PASSED\n");
	sys->print("========================================\n");
}

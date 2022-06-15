const std = @import("std");
const Self = @This();

step: std.build.Step,
builder: *std.build.Builder,
name: []const u8,
script: []const u8,

pub fn create(name: []const u8, b: *std.build.Builder, script: []const u8) *Self {
    var self = b.allocator.create(Self) catch @panic("memory");
    self.* = Self{
        .step = std.build.Step.init(.custom, "run a bash script in a subprocess", b.allocator, build),
        .builder = b,
        .name = name,
        .script = script,
    };
    return self;
}

/// FIXME: Poached from std.ChildProcess because it's private there.
fn collectOutputPosix(
    child: *const std.ChildProcess,
    stdout: *std.ArrayList(u8),
    stderr: *std.ArrayList(u8),
    max_output_bytes: usize,
) !void {
    var poll_fds = [_]std.os.pollfd{
        .{ .fd = child.stdout.?.handle, .events = std.os.POLL.IN, .revents = undefined },
        .{ .fd = child.stderr.?.handle, .events = std.os.POLL.IN, .revents = undefined },
    };

    var dead_fds: usize = 0;
    // We ask for ensureTotalCapacity with this much extra space. This has more of an
    // effect on small reads because once the reads start to get larger the amount
    // of space an ArrayList will allocate grows exponentially.
    const bump_amt = 512;

    const err_mask = std.os.POLL.ERR | std.os.POLL.NVAL | std.os.POLL.HUP;

    while (dead_fds < poll_fds.len) {
        const events = try std.os.poll(&poll_fds, std.math.maxInt(i32));
        if (events == 0) continue;

        var remove_stdout = false;
        var remove_stderr = false;
        // Try reading whatever is available before checking the error
        // conditions.
        // It's still possible to read after a POLL.HUP is received, always
        // check if there's some data waiting to be read first.
        if (poll_fds[0].revents & std.os.POLL.IN != 0) {
            // stdout is ready.
            const new_capacity = std.math.min(stdout.items.len + bump_amt, max_output_bytes);
            try stdout.ensureTotalCapacity(new_capacity);
            const buf = stdout.unusedCapacitySlice();
            if (buf.len == 0) return error.StdoutStreamTooLong;
            const nread = try std.os.read(poll_fds[0].fd, buf);
            stdout.items.len += nread;

            // Remove the fd when the EOF condition is met.
            remove_stdout = nread == 0;
        } else {
            remove_stdout = poll_fds[0].revents & err_mask != 0;
        }

        if (poll_fds[1].revents & std.os.POLL.IN != 0) {
            // stderr is ready.
            const new_capacity = std.math.min(stderr.items.len + bump_amt, max_output_bytes);
            try stderr.ensureTotalCapacity(new_capacity);
            const buf = stderr.unusedCapacitySlice();
            if (buf.len == 0) return error.StderrStreamTooLong;
            const nread = try std.os.read(poll_fds[1].fd, buf);
            stderr.items.len += nread;

            // Remove the fd when the EOF condition is met.
            remove_stderr = nread == 0;
        } else {
            remove_stderr = poll_fds[1].revents & err_mask != 0;
        }

        // Exclude the fds that signaled an error.
        if (remove_stdout) {
            poll_fds[0].fd = -1;
            dead_fds += 1;
        }
        if (remove_stderr) {
            poll_fds[1].fd = -1;
            dead_fds += 1;
        }
    }
}

pub fn build(step: *std.build.Step) !void {
    const self = @fieldParentPtr(Self, "step", step);

    const child = try std.ChildProcess.init(&.{ "bash", "-u" }, self.builder.allocator);
    defer child.deinit();
    child.stdin_behavior = .Pipe;
    child.stdout_behavior = .Pipe;
    child.stderr_behavior = .Pipe;
    child.cwd = self.builder.build_root;
    child.env_map = self.builder.env_map;

    try child.spawn();
    try child.stdin.?.writeAll(self.script);
    try child.stdin.?.writeAll("\n");
    child.stdin.?.close();
    child.stdin = null;

    var stdout = std.ArrayList(u8).init(self.builder.allocator);
    var stderr = std.ArrayList(u8).init(self.builder.allocator);
    try collectOutputPosix(child, &stdout, &stderr, 50 * 1024);

    const result = try child.wait();
    switch (result) {
        .Exited => |code| if (code != 0) {
            std.log.err("STDOUT:\n{s}", .{stdout.items});
            std.log.err("STDERR:\n{s}", .{stderr.items});
            std.log.err("{s} failed with exit code {}", .{ self.name, code });
            std.os.exit(0xff);
        },
        else => {
            std.log.err("STDOUT:\n{s}", .{stdout.items});
            std.log.err("STDERR:\n{s}", .{stderr.items});
            std.log.err("{s} failed with: {}", .{ self.name, result });
            std.os.exit(0xff);
        },
    }
}

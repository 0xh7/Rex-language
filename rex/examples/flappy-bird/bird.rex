// Flappy Bird clone made by 0xh7

use rex::ui
use rex::time
use rex::random
use rex::collections as col
use rex::audio

struct Pipe { x: f64, gap_y: f64, passed: bool }

fn rects_overlap(ax: f64, ay: f64, aw: f64, ah: f64, bx: f64, by: f64, bw: f64, bh: f64) -> bool {
    return ax < (bx + bw) && (ax + aw) > bx && ay < (by + bh) && (ay + ah) > by
}

fn clamp(v: f64, lo: f64, hi: f64) -> f64 {
    if v < lo {
        return lo
    }
    if v > hi {
        return hi
    }
    return v
}

fn main() {
    random.seed(time.now_ms())

    let title = "Rex Flappy Bird"
    let win_w: i32 = 288
    let win_h: i32 = 512

    let bg_day = ui.image_load("examples/flappy-bird/ic/background-day.png")
    let bg_night = ui.image_load("examples/flappy-bird/ic/background-night.png")
    let base_img = ui.image_load("examples/flappy-bird/ic/base.png")
    let bird_up = ui.image_load("examples/flappy-bird/ic/bluebird-upflap.png")
    let bird_mid = ui.image_load("examples/flappy-bird/ic/bluebird-midflap.png")
    let bird_down = ui.image_load("examples/flappy-bird/ic/bluebird-downflap.png")
    let pipe_img = ui.image_load("examples/flappy-bird/ic/pipe-red.png")

    let digits: Vec<any> = [
        ui.image_load("examples/flappy-bird/ic/0.png"),
        ui.image_load("examples/flappy-bird/ic/1.png"),
        ui.image_load("examples/flappy-bird/ic/2.png"),
        ui.image_load("examples/flappy-bird/ic/3.png"),
        ui.image_load("examples/flappy-bird/ic/4.png"),
        ui.image_load("examples/flappy-bird/ic/5.png"),
        ui.image_load("examples/flappy-bird/ic/6.png"),
        ui.image_load("examples/flappy-bird/ic/7.png"),
        ui.image_load("examples/flappy-bird/ic/8.png"),
        ui.image_load("examples/flappy-bird/ic/9.png")
    ]

    let sfx_wing = "examples/flappy-bird/sound/sfx_wing.wav"
    let sfx_hit = "examples/flappy-bird/sound/sfx_hit.wav"
    let sfx_die = "examples/flappy-bird/sound/sfx_die.wav"
    let sfx_point = "examples/flappy-bird/sound/sfx_point.wav"
    let sfx_swoosh = "examples/flappy-bird/sound/sfx_swooshing.wav"

    let base_w: f64 = ui.image_w(&base_img)
    let base_h: f64 = ui.image_h(&base_img)
    let bird_w: f64 = ui.image_w(&bird_mid)
    let bird_h: f64 = ui.image_h(&bird_mid)
    let pipe_w: f64 = ui.image_w(&pipe_img)
    let pipe_h: f64 = ui.image_h(&pipe_img)

    let ground_y: f64 = win_h - base_h

    let gravity: f64 = 0.35
    let flap_vy: f64 = -5.8
    let max_fall: f64 = 8.5
    let pipe_speed: f64 = 2.0
    let pipe_gap: f64 = 100
    let pipe_spacing: f64 = 160
    let pipe_count: f64 = 3
    let tilt_up: f64 = 25
    let tilt_down: f64 = -90
    let tilt_scale: f64 = 10

    let gap_min: f64 = 60
    let gap_max: f64 = clamp(ground_y - pipe_gap - 60, 80, ground_y - pipe_gap)

    mut state: i32 = 0
    mut score: f64 = 0
    mut best: f64 = 0
    mut played_die: bool = false

    mut bird_x: f64 = win_w * 0.32
    mut bird_y: f64 = win_h * 0.4
    mut bird_vy: f64 = 0
    mut bird_rot: f64 = 0
    mut anim_timer: f64 = 0
    mut anim_frame: i32 = 0
    mut idle_offset: f64 = 0
    mut idle_dir: f64 = 1

    mut ground_x: f64 = 0
    mut spawn_x: f64 = win_w + 120
    mut pipes = col.vec_new<Pipe>()
    mut flap_queued: bool = false

    mut last = time.now_ms()
    mut acc: f64 = 0

    while ui.begin(&title, win_w, win_h) {
        ui.redraw()

        let now = time.now_ms()
        mut dt = now - last
        if dt < 0 {
            dt = 0
        }
        if dt > 80 {
            dt = 80
        }
        last = now 
        acc = acc + dt

        if ui.key_space() || ui.mouse_pressed() {
            flap_queued = true
        }

        while acc >= 16 {
            acc = acc - 16

            if state != 2 {
                anim_timer = anim_timer + 16
                if anim_timer >= 90 {
                    anim_timer = 0
                    anim_frame = (anim_frame + 1) % 3
                }
            }

            if state == 0 {
                idle_offset = idle_offset + idle_dir * 0.25
                if idle_offset > 6 {
                    idle_offset = 6
                    idle_dir = -1
                }
                if idle_offset < -6 {
                    idle_offset = -6
                    idle_dir = 1
                }
                bird_y = win_h * 0.4 + idle_offset
                bird_vy = 0
                bird_rot = 0
                if flap_queued {
                    state = 1
                    score = 0
                    bird_x = win_w * 0.32
                    bird_y = win_h * 0.4
                    bird_vy = flap_vy
                    bird_rot = tilt_up
                    anim_frame = 0
                    anim_timer = 0
                    idle_offset = 0
                    ground_x = 0
                    pipes = col.vec_new<Pipe>()
                    spawn_x = win_w + 120
                    mut i: f64 = 0
                    while i < pipe_count {
                        let gap = random.int(gap_min, gap_max)
                        col.vec_push(&mut pipes, Pipe.new(spawn_x, gap, false))
                        spawn_x = spawn_x + pipe_spacing
                        i = i + 1
                    }
                    audio.play(&sfx_swoosh)
                    played_die = false
                    flap_queued = false
                }
            } else if state == 1 {
                if flap_queued {
                    bird_vy = flap_vy
                    bird_rot = tilt_up
                    audio.play(&sfx_wing)
                    flap_queued = false
                }

                bird_vy = bird_vy + gravity
                if bird_vy > max_fall {
                    bird_vy = max_fall
                }
                bird_y = bird_y + bird_vy
                bird_rot = clamp(-bird_vy * tilt_scale, tilt_down, tilt_up)

                ground_x = ground_x - pipe_speed
                if ground_x <= -base_w {
                    ground_x = ground_x + base_w
                }

                mut next_pipes = col.vec_new<Pipe>()
                let n = col.vec_len(&pipes)
                mut i: f64 = 0
                while i < n {
                    let p = col.vec_get(&pipes, i)
                    let nx = p.x - pipe_speed
                    if (nx + pipe_w) <= 0 {
                        let gap = random.int(gap_min, gap_max)
                        col.vec_push(&mut next_pipes, Pipe.new(spawn_x, gap, false))
                        spawn_x = spawn_x + pipe_spacing
                    } else {
                        mut passed = p.passed
                        if !passed && (nx + pipe_w) < bird_x {
                            passed = true
                            score = score + 1
                            audio.play(&sfx_point)
                        }
                        col.vec_push(&mut next_pipes, Pipe.new(nx, p.gap_y, passed))
                    }
                    i = i + 1
                }
                pipes = next_pipes

                mut hit_ground: bool = false
                mut hit_pipe: bool = false
                if bird_y < 0 || (bird_y + bird_h) >= ground_y {
                    hit_ground = true
                }

                let pn = col.vec_len(&pipes)
                mut pi: f64 = 0
                while pi < pn {
                    let p = col.vec_get(&pipes, pi)
                    let top_x = p.x
                    let top_y = p.gap_y - pipe_h
                    let bot_x = p.x
                    let bot_y = p.gap_y + pipe_gap
                    if rects_overlap(bird_x, bird_y, bird_w, bird_h, top_x, top_y, pipe_w, pipe_h) {
                        hit_pipe = true
                    }
                    if rects_overlap(bird_x, bird_y, bird_w, bird_h, bot_x, bot_y, pipe_w, pipe_h) {
                        hit_pipe = true
                    }
                    pi = pi + 1
                }

                if hit_pipe || hit_ground {
                    state = 2
                    if score > best {
                        best = score
                    }
                    anim_frame = 1
                    anim_timer = 0
                    if hit_pipe {
                        audio.play(&sfx_hit)
                    }
                    if hit_ground {
                        audio.play(&sfx_die)
                        played_die = true
                    } else {
                        played_die = false
                    }
                }
            } else {
                if bird_y < (ground_y - bird_h) {
                    bird_vy = bird_vy + gravity
                    if bird_vy > max_fall {
                        bird_vy = max_fall
                    }
                    bird_y = bird_y + bird_vy
                }
                if bird_y >= (ground_y - bird_h) {
                    bird_y = ground_y - bird_h
                    bird_vy = 0
                    bird_rot = tilt_down
                    if !played_die {
                        audio.play(&sfx_die)
                        played_die = true
                    }
                } else {
                    bird_rot = clamp(-bird_vy * tilt_scale, tilt_down, tilt_up)
                }
                if flap_queued {
                    state = 1
                    score = 0
                    bird_x = win_w * 0.32
                    bird_y = win_h * 0.4
                    bird_vy = flap_vy
                    bird_rot = tilt_up
                    anim_frame = 0
                    anim_timer = 0
                    idle_offset = 0
                    ground_x = 0
                    pipes = col.vec_new<Pipe>()
                    spawn_x = win_w + 120
                    mut i: f64 = 0
                    while i < pipe_count {
                        let gap = random.int(gap_min, gap_max)
                        col.vec_push(&mut pipes, Pipe.new(spawn_x, gap, false))
                        spawn_x = spawn_x + pipe_spacing
                        i = i + 1
                    }
                    audio.play(&sfx_swoosh)
                    played_die = false
                    flap_queued = false
                }
            }
        }

        ui.clear("#4ec0ca")

        let night = score >= 10 && (score % 20) >= 10
        if night {
            ui.image(&bg_night, 0, 0)
        } else {
            ui.image(&bg_day, 0, 0)
        }

        let pn = col.vec_len(&pipes)
        mut pi: f64 = 0
        while pi < pn {
            let p = col.vec_get(&pipes, pi)
            let top_x = p.x
            let top_y = p.gap_y - pipe_h
            let bot_x = p.x
            let bot_y = p.gap_y + pipe_gap
            ui.image_rot(&pipe_img, top_x + (pipe_w / 2), top_y + (pipe_h / 2), 180)
            ui.image(&pipe_img, bot_x, bot_y)
            pi = pi + 1
        }

        mut gx = ground_x
        while gx < win_w {
            ui.image(&base_img, gx, ground_y)
            gx = gx + base_w
        }

        let bird_cx = bird_x + (bird_w / 2)
        let bird_cy = bird_y + (bird_h / 2)
        if anim_frame == 0 {
            ui.image_rot(&bird_up, bird_cx, bird_cy, bird_rot)
        } else if anim_frame == 1 {
            ui.image_rot(&bird_mid, bird_cx, bird_cy, bird_rot)
        } else {
            ui.image_rot(&bird_down, bird_cx, bird_cy, bird_rot)
        }

        let digit_spacing: f64 = 2
        mut tmp = score
        if tmp < 0 { tmp = 0 }
        mut digits_vec = col.vec_new<f64>()
        if tmp == 0 {
            col.vec_push(&mut digits_vec, 0)
        } else {
            while tmp > 0 {
                let d = tmp % 10
                col.vec_push(&mut digits_vec, d)
                tmp = (tmp - d) / 10
            }
        }

        let dn = col.vec_len(&digits_vec)
        mut total_w: f64 = 0
        mut i: f64 = dn - 1
        while i >= 0 {
            let d = col.vec_get(&digits_vec, i)
            let img = col.vec_get(&digits, d)
            total_w = total_w + ui.image_w(&img)
            if i > 0 {
                total_w = total_w + digit_spacing
            }
            if i == 0 { break }
            i = i - 1
        }

        mut x = (win_w - total_w) / 2
        let y: f64 = 20
        i = dn - 1
        while i >= 0 {
            let d = col.vec_get(&digits_vec, i)
            let img = col.vec_get(&digits, d)
            ui.image(&img, x, y)
            x = x + ui.image_w(&img) + digit_spacing
            if i == 0 { break }
            i = i - 1
        }

        if state == 0 {
            ui.text((win_w - 130) / 2, (win_h / 2) - 20, "Press Space", "#ffffff")
        } else if state == 2 {
            ui.text((win_w - 110) / 2, (win_h / 2) - 30, "Game Over", "#ffffff")
            ui.text((win_w - 170) / 2, (win_h / 2), "Space to restart", "#ffffff")
        }
        ui.end()
    }
}

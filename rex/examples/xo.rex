// Tic-tac-toe (XO) with minimax AI.

use rex::ui
use rex::collections as col

fn check_winner(&board: Vec<f64>) -> f64 {
    let a0 = col.vec_get(board, 0)
    let a1 = col.vec_get(board, 1)
    let a2 = col.vec_get(board, 2)
    let b0 = col.vec_get(board, 3)
    let b1 = col.vec_get(board, 4)
    let b2 = col.vec_get(board, 5)
    let c0 = col.vec_get(board, 6)
    let c1 = col.vec_get(board, 7)
    let c2 = col.vec_get(board, 8)

    if a0 != 0 && a0 == a1 && a1 == a2 { return a0 }
    if b0 != 0 && b0 == b1 && b1 == b2 { return b0 }
    if c0 != 0 && c0 == c1 && c1 == c2 { return c0 }
    if a0 != 0 && a0 == b0 && b0 == c0 { return a0 }
    if a1 != 0 && a1 == b1 && b1 == c1 { return a1 }
    if a2 != 0 && a2 == b2 && b2 == c2 { return a2 }
    if a0 != 0 && a0 == b1 && b1 == c2 { return a0 }
    if a2 != 0 && a2 == b1 && b1 == c0 { return a2 }
    return 0
}

fn board_full(&board: Vec<f64>) -> bool {
    mut i: f64 = 0
    while i < 9 {
        if col.vec_get(board, i) == 0 {
            return false
        }
        i = i + 1
    }
    return true
}

fn minimax(&mut board: Vec<f64>, player: f64, depth: f64) -> f64 {
    let winner = check_winner(board)
    if winner == 2 {
        return 10 - depth
    }
    if winner == 1 {
        return depth - 10
    }
    if board_full(board) {
        return 0
    }

    if player == 2 {
        mut best: f64 = -1000
        mut i: f64 = 0
        while i < 9 {
            if col.vec_get(board, i) == 0 {
                col.vec_set(board, i, 2)
                let score = minimax(board, 1, depth + 1)
                col.vec_set(board, i, 0)
                if score > best {
                    best = score
                }
            }
            i = i + 1
        }
        return best
    }

    mut best: f64 = 1000
    mut i: f64 = 0
    while i < 9 {
        if col.vec_get(board, i) == 0 {
            col.vec_set(board, i, 1)
            let score = minimax(board, 2, depth + 1)
            col.vec_set(board, i, 0)
            if score < best {
                best = score
            }
        }
        i = i + 1
    }
    return best
}

fn ai_pick(&mut board: Vec<f64>) -> f64 {
    mut best: f64 = -1000
    mut best_idx: f64 = -1
    mut i: f64 = 0
    while i < 9 {
        if col.vec_get(board, i) == 0 {
            col.vec_set(board, i, 2)
            let score = minimax(board, 1, 0)
            col.vec_set(board, i, 0)
            if score > best {
                best = score
                best_idx = i
            }
        }
        i = i + 1
    }
    return best_idx
}

fn main() {
    let title = "Rex XO"
    let win_w: i32 = 360
    let win_h: i32 = 480
    let cell: f64 = 96
    let spacing: f64 = 8
    let padding: f64 = 16

    let bg = "#F5F1E6"
    let panel = "#EFE4CF"
    let text = "#2A2A2A"
    let muted = "#6B6B6B"
    let button = "#E7D6BF"
    let button_hover = "#D9C4A7"
    let button_active = "#C9B091"
    let accent = "#1F8A8A"
    let select = "#BEE4E4"

    mut board: Vec<f64> = [0, 0, 0, 0, 0, 0, 0, 0, 0]
    mut state: f64 = 0
    mut winner: f64 = 0
    mut turn: f64 = 1
    mut ai_first: bool = false

    while ui.begin(&title, win_w, win_h) {
        ui.redraw()
        ui.theme_custom(&bg, &panel, &text, &muted, &button, &button_hover, &button_active, &accent, &select)
        ui.spacing(spacing)
        ui.padding(padding)
        ui.clear(&bg)

        mut status = "Press New Game"
        if state == 1 {
            if turn == 1 {
                status = "Your turn (X)"
            } else {
                status = "AI thinking..."
            }
        } else if state == 2 {
            if winner == 1 {
                status = "You win!"
            } else if winner == 2 {
                status = "AI wins!"
            } else {
                status = "Draw!"
            }
        }

        ui.label(&title)
        ui.label(&status)
        ai_first = ui.checkbox("AI starts", ai_first)
        if ui.button("New Game") {
            board = [0, 0, 0, 0, 0, 0, 0, 0, 0]
            state = 1
            winner = 0
            if ai_first {
                turn = 2
            } else {
                turn = 1
            }
        }

        ui.newline()

        let allow_play = state == 1 && turn == 1
        ui.enabled(allow_play)

        ui.grid(3, cell, cell)
        mut i: f64 = 0
        mut human_moved: bool = false
        while i < 9 {
            let cell_val = col.vec_get(&board, i)
            mut label = ""
            if cell_val == 1 {
                label = "X"
            } else if cell_val == 2 {
                label = "O"
            }
            if ui.button(&label) && allow_play && cell_val == 0 {
                col.vec_set(&mut board, i, 1)
                human_moved = true
            }
            i = i + 1
        }
        ui.enabled(true)

        if human_moved {
            let w = check_winner(&board)
            if w != 0 {
                winner = w
                state = 2
            } else if board_full(&board) {
                winner = 3
                state = 2
            } else {
                turn = 2
            }
        }

        if state == 1 && turn == 2 {
            let mv = ai_pick(&mut board)
            if mv >= 0 {
                col.vec_set(&mut board, mv, 2)
            }
            let w = check_winner(&board)
            if w != 0 {
                winner = w
                state = 2
            } else if board_full(&board) {
                winner = 3
                state = 2
            } else {
                turn = 1
            }
        }
        ui.end()
    }
}

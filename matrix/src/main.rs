// 실습 2: 강의자료 6-3장 - Vec<T>, let mut, for, Cargo를 사용한 행렬 덧셈
use std::io::{self, Write};

fn read_number(prompt: &str) -> usize {
    loop {
        print!("{}", prompt);
        io::stdout().flush().unwrap();
        let mut line = String::new();
        io::stdin().read_line(&mut line).unwrap();
        if let Ok(value) = line.trim().parse::<usize>() {
            if value > 0 && value <= 100 { return value; }
        }
        println!("1부터 100까지의 정수를 입력하세요.");
    }
}

fn read_element(prompt: &str) -> i32 {
    loop {
        print!("{}", prompt);
        io::stdout().flush().unwrap();
        let mut line = String::new();
        io::stdin().read_line(&mut line).unwrap();
        if let Ok(value) = line.trim().parse::<i32>() { return value; }
        println!("정수를 다시 입력하세요.");
    }
}

fn main() {
    let rows = read_number("행 수: ");
    let cols = read_number("열 수: ");
    let mut a: Vec<Vec<i32>> = vec![vec![0; cols]; rows];
    let mut b: Vec<Vec<i32>> = vec![vec![0; cols]; rows];
    let mut result: Vec<Vec<i32>> = vec![vec![0; cols]; rows];
    for i in 0..rows {
        for j in 0..cols {
            a[i][j] = read_element(&format!("A[{}][{}]: ", i, j));
        }
    }
    for i in 0..rows {
        for j in 0..cols {
            b[i][j] = read_element(&format!("B[{}][{}]: ", i, j));
        }
    }
    println!("A + B =");
    for i in 0..rows {
        for j in 0..cols {
            if let Some(v) = a[i][j].checked_add(b[i][j]) {
                result[i][j] = v;
                print!("{:6}", result[i][j]);
            } else {
                eprintln!("정수 덧셈 범위 초과");
                return;
            }
        }
        println!();
    }
}

#!/usr/bin/env python3
"""
Генератор загрузчика glad для проекта.
Результат сохраняется в thirdparty/glad/
"""
import subprocess
import sys
from pathlib import Path

def main():
    project_root = Path(__file__).parent.parent
    output_dir = project_root / "thirdparty" / "glad"

    # Очищаем старые файлы
    if output_dir.exists():
        import shutil
        shutil.rmtree(output_dir)

    output_dir.mkdir(parents=True, exist_ok=True)

    # Проверяем, установлен ли glad-generator
    try:
        import glad
    except ImportError:
        print("Устанавливаю glad-generator...")
        subprocess.check_call([sys.executable, "-m", "pip", "install", "glad>=2.0"])
        import glad

    print("Генерация glad для OpenGL 4.6 Core...")

    # Вызываем glad как модуль
    from glad.__main__ import main as glad_main

    old_argv = sys.argv
    sys.argv = [
        "glad",
        "--api=gl=4.6",
        "--profile=core",
        "--generator=c",
        f"--out-path={output_dir}",
        "--reproducible",
        "--no-loader"
    ]

    try:
        glad_main()
        print(f"glad сгенерирован в {output_dir}")
        print("Структура:")
        for f in sorted(output_dir.rglob("*")):
            if f.is_file():
                print(f"  {f.relative_to(output_dir)}")
    finally:
        sys.argv = old_argv

if __name__ == "__main__":
    main()

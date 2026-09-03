#!/bin/bash

GDEXTENSION_FILE="godot_recycler_view.gdextension"

if [[ ! -f "$GDEXTENSION_FILE" ]]; then
    echo "错误: 未找到 $GDEXTENSION_FILE 文件"
    exit 1
fi

echo "正在检查 $GDEXTENSION_FILE 中的库引用..."

TMP_LIST=$(mktemp)

# 提取路径并去掉开头的 "./"
awk '/^\[libraries\]/ {flag=1; next} /^\[/ {flag=0} flag && !/^#/ && NF>0' "$GDEXTENSION_FILE" | \
while IFS='=' read -r key value; do
    path=$(echo "$value" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//' -e 's/^"//' -e 's/"$//')
    # 去掉开头的 "./"
    clean_path=$(echo "$path" | sed 's#^\./##')
    if [[ -n "$clean_path" ]]; then
        echo "$clean_path"
    fi
done > "$TMP_LIST"

# 读入数组
LISTED_PATHS=()
while IFS= read -r line; do
    LISTED_PATHS+=("$line")
done < "$TMP_LIST"

# 检查列出的文件是否存在
MISSING=0
for p in "${LISTED_PATHS[@]}"; do
    # 注意：p 已无 ./，但实际文件也可能无 ./，所以直接用 p 检查（如果文件在子目录）
    if [[ ! -f "$p" ]]; then
        echo "❌ 缺失: $p"
        MISSING=1
    fi
done

if [[ $MISSING -eq 0 ]]; then
    echo "✅ 所有列出的库文件均存在。"
fi

# 收集所有唯一目录（来自列出的路径）
UNIQUE_DIRS=()
for p in "${LISTED_PATHS[@]}"; do
    dir=$(dirname "$p")
    # 去重
    found=0
    for d in "${UNIQUE_DIRS[@]}"; do
        if [[ "$d" == "$dir" ]]; then
            found=1
            break
        fi
    done
    if [[ $found -eq 0 && -d "$dir" ]]; then
        UNIQUE_DIRS+=("$dir")
    fi
done

# 扫描这些目录，与列表比对
EXTRA_FOUND=0
for dir in "${UNIQUE_DIRS[@]}"; do
    find "$dir" -maxdepth 1 -type f \( -name "*.dylib" -o -name "*.so" -o -name "*.dll" -o -name "*.wasm" \) | while read -r file; do
        # 去掉开头的 "./"（如果有）
        rel_file="${file#./}"
        # 检查是否在列表中（列表已无 ./）
        found=0
        for listed in "${LISTED_PATHS[@]}"; do
            if [[ "$rel_file" == "$listed" ]]; then
                found=1
                break
            fi
        done
        if [[ $found -eq 0 ]]; then
            echo "⚠️  未列举但存在的库: $rel_file"
            EXTRA_FOUND=1
        fi
    done
done

if [[ $EXTRA_FOUND -eq 0 ]]; then
    echo "✅ 未发现多余的库文件。"
fi

rm -f "$TMP_LIST"
echo "检查完成。"

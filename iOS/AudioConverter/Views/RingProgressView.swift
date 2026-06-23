//
//  RingProgressView.swift
//  AudioConverter
//
//  Created by wxj on 2026/6/18.
//


import SwiftUI

struct RingProgressView: View {
    let progress: Double // 0 ~ 1
    let ringWidth: CGFloat
    let ringColor: Color
    let bgRingColor: Color
    
    var body: some View {
        ZStack {
            // 底层灰色背景圆环
            Circle()
                .stroke(
                    bgRingColor,
                    lineWidth: ringWidth
                )
            
            // 上层绿色进度圆环
            Circle()
                .trim(from: 0, to: progress)
                .stroke(
                    ringColor,
                    style: StrokeStyle(
                        lineWidth: ringWidth,
                        lineCap: .round
                    )
                )
                .rotationEffect(.degrees(-90)) // 从顶部开始绘制
        }
    }
}

// 预览
struct RingProgressView_Previews: PreviewProvider {
    static var previews: some View {
        RingProgressView(
            progress: 0.46,
            ringWidth: 12,
            ringColor: Color.green,
            bgRingColor: Color.gray.opacity(0.3)
        )
        .frame(width: 220, height: 220)
    }
}

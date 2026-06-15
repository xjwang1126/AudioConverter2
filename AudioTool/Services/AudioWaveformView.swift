import SwiftUI

struct AudioWaveformView: View {
    let audioLevels: [Float]
    var barColor: Color = .accentColor
    var barCount: Int = 60
    
    var body: some View {
        GeometryReader { geometry in
            let availableWidth = geometry.size.width
            let barWidth = availableWidth / CGFloat(barCount)
            let levels = interpolateLevels(from: audioLevels, to: barCount)
            
            HStack(alignment: .center, spacing: 1) {
                ForEach(levels.indices, id: \.self) { index in
                    let height = max(CGFloat(levels[index]) * geometry.size.height * 0.8, 2)
                    RoundedRectangle(cornerRadius: 2)
                        .fill(barColor.gradient)
                        .frame(width: barWidth - 1, height: height)
                }
            }
            .frame(maxHeight: .infinity, alignment: .center)
        }
    }
    
    private func interpolateLevels(from levels: [Float], to count: Int) -> [Float] {
        guard !levels.isEmpty else {
            return Array(repeating: 0.05, count: count)
        }
        
        if levels.count >= count {
            return stride(from: 0, to: levels.count, by: levels.count / count).map { levels[min($0, levels.count - 1)] }
        }
        
        var result: [Float] = []
        let step = Float(levels.count) / Float(count)
        var pos: Float = 0
        
        for _ in 0..<count {
            let idx = min(Int(pos), levels.count - 1)
            result.append(levels[idx])
            pos += step
        }
        
        return result
    }
}

#Preview {
    AudioWaveformView(audioLevels: [0.1, 0.3, 0.5, 0.8, 0.6, 0.4, 0.2])
        .frame(height: 100)
        .padding()
}

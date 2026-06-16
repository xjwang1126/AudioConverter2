import Foundation
import AVFoundation
import Combine

class AudioPlayer: NSObject, ObservableObject {
    private var player: AVAudioPlayer?
    
    @Published var isPlaying = false
    @Published var currentTime: TimeInterval = 0
    @Published var duration: TimeInterval = 0
    @Published var playbackProgress: Double = 0
    
    private var timer: Timer?
    private var currentURL: URL?
    
    var currentFileURL: URL? {
        currentURL
    }
    
    func play(url: URL) {
        stop()
        
        guard let player = try? AVAudioPlayer(contentsOf: url) else { return }
        self.player = player
        player.delegate = self
        player.prepareToPlay()
        
        currentURL = url
        duration = player.duration
        player.play()
        isPlaying = true
        
        startProgressTimer()
    }
    
    func playPause() {
        guard let player = player else { return }
        
        if player.isPlaying {
            player.pause()
            isPlaying = false
            timer?.invalidate()
        } else {
            player.play()
            isPlaying = true
            startProgressTimer()
        }
    }
    
    func stop() {
        player?.stop()
        player = nil
        isPlaying = false
        currentTime = 0
        playbackProgress = 0
        timer?.invalidate()
        timer = nil
    }
    
    func seek(to progress: Double) {
        guard let player = player else { return }
        let time = progress * duration
        player.currentTime = time
        currentTime = time
    }
    
    private func startProgressTimer() {
        timer?.invalidate()
        timer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { [weak self] _ in
            guard let self = self, let player = self.player else { return }
            DispatchQueue.main.async {
                self.currentTime = player.currentTime
                self.playbackProgress = player.currentTime / max(player.duration, 1)
            }
        }
    }
}

// MARK: - AVAudioPlayerDelegate
extension AudioPlayer: AVAudioPlayerDelegate {
    func audioPlayerDidFinishPlaying(_ player: AVAudioPlayer, successfully flag: Bool) {
        DispatchQueue.main.async {
            self.isPlaying = false
            self.currentTime = 0
            self.playbackProgress = 0
            self.timer?.invalidate()
            self.timer = nil
        }
    }
}

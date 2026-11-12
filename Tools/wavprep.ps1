# Prepares a recording for the game: 48 kHz, 16-bit, optional trim, optional seamless loop.
# usage: powershell -File Tools/wavprep.ps1 -In src.wav -Out dst.wav [-Start 5] [-Seconds 60] [-Loop] [-Mono]
# Reads PCM WAV of 16/24/32-bit integer or 32-bit float at 48, 96 or 192 kHz (plain or extensible header).
param([string]$In, [string]$Out, [double]$Start = 0, [double]$Seconds = 0, [switch]$Loop, [switch]$Mono)

Add-Type -TypeDefinition @"
using System;
using System.IO;

public static class WavPrep
{
    public static string Convert(string input, string output, double start, double seconds, bool loop, bool mono)
    {
        byte[] b = File.ReadAllBytes(input);
        int pos = 12, channels = 0, rate = 0, bits = 0, format = 1, dataPos = -1, dataLen = 0;
        while (pos + 8 <= b.Length)
        {
            string id = System.Text.Encoding.ASCII.GetString(b, pos, 4);
            int size = BitConverter.ToInt32(b, pos + 4);
            if (id == "fmt ")
            {
                format = BitConverter.ToUInt16(b, pos + 8);
                channels = BitConverter.ToUInt16(b, pos + 10);
                rate = BitConverter.ToInt32(b, pos + 12);
                bits = BitConverter.ToUInt16(b, pos + 22);
                if (format == 0xFFFE && size >= 26) { format = BitConverter.ToUInt16(b, pos + 32); }
            }
            else if (id == "data") { dataPos = pos + 8; dataLen = Math.Min(size, b.Length - dataPos); break; }
            pos += 8 + size + (size & 1);
        }
        if (dataPos < 0 || channels == 0) { return "not a readable wav"; }
        int step = Math.Max(1, rate / 48000);
        int outRate = rate / step;
        int bytesPer = bits / 8;
        int frameBytes = bytesPer * channels;
        long frames = dataLen / frameBytes;

        long first = (long)(start * rate);
        double fade = loop ? 2.0 : 0.0;
        long want = seconds > 0 ? (long)((seconds + fade) * rate) : frames - first;
        if (first + want > frames) { want = frames - first; }
        int outFrames = (int)(want / step);
        int outCh = mono ? 1 : Math.Min(channels, 2);
        float[] s = new float[outFrames * outCh];

        for (int f = 0; f < outFrames; f++)
        {
            for (int c = 0; c < outCh; c++)
            {
                // Average the frames being merged (a crude low-pass before dropping the sample rate),
                // and the channels being merged when going to mono.
                double acc = 0; int n = 0;
                for (int k = 0; k < step; k++)
                {
                    long frame = first + (long)f * step + k;
                    int chFrom = mono ? 0 : c, chTo = mono ? Math.Min(channels, 2) - 1 : c;
                    for (int ch = chFrom; ch <= chTo; ch++)
                    {
                        int p = dataPos + (int)(frame * frameBytes) + ch * bytesPer;
                        double v;
                        if (format == 3) { v = BitConverter.ToSingle(b, p); }
                        else if (bits == 16) { v = BitConverter.ToInt16(b, p) / 32768.0; }
                        else if (bits == 24) { v = ((b[p] | (b[p + 1] << 8) | ((sbyte)b[p + 2] << 16))) / 8388608.0; }
                        else { v = BitConverter.ToInt32(b, p) / 2147483648.0; }
                        acc += v; n++;
                    }
                }
                s[f * outCh + c] = (float)(acc / n);
            }
        }

        int keep = outFrames;
        if (loop)
        {
            // Fold the last two seconds into the first two so the end meets the start without a click.
            int x = (int)(fade * outRate);
            keep = outFrames - x;
            for (int i = 0; i < x; i++)
            {
                float t = (float)i / x;
                for (int c = 0; c < outCh; c++) { s[i * outCh + c] = s[i * outCh + c] * t + s[(keep + i) * outCh + c] * (1 - t); }
            }
        }
        else
        {
            // One-shots get a short fade at each end.
            int x = Math.Min(outRate / 50, keep / 4);
            for (int i = 0; i < x; i++)
            {
                float t = (float)i / x;
                for (int c = 0; c < outCh; c++) { s[i * outCh + c] *= t; s[(keep - 1 - i) * outCh + c] *= t; }
            }
        }

        float peak = 0.0001f;
        for (int i = 0; i < keep * outCh; i++) { peak = Math.Max(peak, Math.Abs(s[i])); }
        float gain = Math.Min(0.9f / peak, 8.0f);

        using (var w = new BinaryWriter(File.Create(output)))
        {
            int bytes = keep * outCh * 2;
            w.Write(System.Text.Encoding.ASCII.GetBytes("RIFF")); w.Write(36 + bytes);
            w.Write(System.Text.Encoding.ASCII.GetBytes("WAVEfmt ")); w.Write(16); w.Write((short)1); w.Write((short)outCh);
            w.Write(outRate); w.Write(outRate * outCh * 2); w.Write((short)(outCh * 2)); w.Write((short)16);
            w.Write(System.Text.Encoding.ASCII.GetBytes("data")); w.Write(bytes);
            for (int i = 0; i < keep * outCh; i++) { w.Write((short)Math.Max(-32767, Math.Min(32767, s[i] * gain * 32767f))); }
        }
        return string.Format("{0}: {1} Hz {2}-bit {3}ch -> {4} Hz 16-bit {5}ch, {6:F1} s, peak {7:F2}, gain x{8:F1}",
            Path.GetFileName(output), rate, bits, channels, outRate, outCh, (double)keep / outRate, peak, gain);
    }
}
"@

Write-Output ([WavPrep]::Convert($In, $Out, $Start, $Seconds, [bool]$Loop, [bool]$Mono))

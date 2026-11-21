// ============================================================================
//  AURORA SYNTH  -  synthé polyphonique (VST3) : 2 oscillateurs + unison + FM + sub,
//  filtre résonant, 2 enveloppes, LFO, drive, chorus, delay ping-pong sync BPM, reverb.
//  60 presets classés en 12 catégories + bouton MUTATE (variations infinies).
// ============================================================================
#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>
#include <vector>
#include <functional>
using namespace juce;

// ----------------------------------------------------------------------------
//  Paramètres
// ----------------------------------------------------------------------------
struct PD { const char* id; const char* name; float mn, mx, def, centre, step; };
static const PD kParams[] = {
    {"w1",  "Osc 1",    0, 4, 2, 0, 1},        //  0
    {"w2",  "Osc 2",    0, 4, 2, 0, 1},        //  1
    {"semi","Semi",   -24, 24, 0, 0, 1},       //  2
    {"det", "Detune",  -50, 50, 7, 0, 0.1f},   //  3
    {"mix", "Osc2 Mix", 0, 1, .5f, 0, 0},      //  4
    {"fm",  "FM",       0, 1, 0, 0, 0},        //  5
    {"uni", "Unison",   1, 5, 1, 0, 1},        //  6
    {"spr", "Spread",   0, 1, .3f, 0, 0},      //  7
    {"sub", "Sub",      0, 1, 0, 0, 0},        //  8
    {"cut", "Cutoff",  40, 18000, 8000, 1200, 0}, // 9
    {"res", "Reso",     0, .95f, .2f, 0, 0},   // 10
    {"env", "Flt Env", -1, 1, .3f, 0, 0},      // 11
    {"A",   "Attack",  .001f, 8, .005f, .4f, 0},  // 12
    {"D",   "Decay",   .005f, 8, .4f, .5f, 0},    // 13
    {"S",   "Sustain",  0, 1, .7f, 0, 0},         // 14
    {"R",   "Release", .005f, 10, .3f, .6f, 0},   // 15
    {"fA",  "F Attack",.001f, 8, .005f, .4f, 0},  // 16
    {"fD",  "F Decay", .01f, 8, .4f, .5f, 0},     // 17
    {"lfoR","LFO Rate",.05f, 20, 4, 2, 0},        // 18
    {"lfoD","LFO>Flt",  0, 1, 0, 0, 0},           // 19
    {"vib", "Vibrato",  0, 1, 0, 0, 0},           // 20
    {"chor","Chorus",   0, 1, .2f, 0, 0},         // 21
    {"dly", "Delay",    0, 1, 0, 0, 0},           // 22
    {"rev", "Reverb",   0, 1, .2f, 0, 0},         // 23
    {"drv", "Drive",    0, 1, 0, 0, 0},           // 24
    {"vol", "Level",    0, 1, .7f, 0, 0}          // 25
};
static constexpr int kNumParams = 26;   // total
static constexpr int kPV = 25;          // valeurs par preset (tout sauf Level)
static const char* kWaves[] = { "Sine", "Triangle", "Saw", "Square", "Noise" };

// ----------------------------------------------------------------------------
//  Presets : nom, catégorie, 25 valeurs dans l'ordre des paramètres 0..24
// ----------------------------------------------------------------------------
struct Preset { const char* name; const char* cat; const char* v; };
static const Preset kPresets[] = {
 // ---- LEAD
 {"Neon Razor","Lead","2 2 0 9 .6 0 3 .5 0 5000 .25 .3 .005 .3 .8 .25 .005 .5 5 0 .15 .3 .2 .25 .1"},
 {"Solar Sine","Lead","0 1 12 3 .25 0 1 0 0 9000 .1 .1 .02 .3 .9 .3 .01 .3 5.5 0 .25 .2 .25 .3 0"},
 {"Hyper Square","Lead","3 2 0 12 .4 0 2 .4 .2 3500 .3 .4 .003 .25 .7 .2 .003 .35 6 0 .1 .2 .15 .2 .25"},
 {"Glass FM Lead","Lead","0 0 12 0 .3 .5 1 0 0 12000 .1 .2 .005 .5 .6 .3 .005 .6 5 0 .2 .25 .25 .3 0"},
 {"Acid Wire","Lead","2 3 0 5 .3 0 1 0 .3 1500 .7 .8 .001 .2 .5 .1 .001 .2 4 0 0 0 .2 .1 .4"},
 // ---- PAD
 {"Aurora Veil","Pad","2 2 0 12 .7 0 5 .8 0 2500 .2 .2 1.2 1 .8 2.5 1.5 2 .3 .3 0 .6 .3 .6 0"},
 {"Deep Space","Pad","1 2 -12 8 .5 0 4 .7 .3 1200 .3 .3 2.5 2 .8 4 3 3 .15 .4 0 .5 .4 .8 0"},
 {"Crystal Choir","Pad","0 1 12 6 .6 .15 3 .5 0 6000 .1 .1 .8 1 .85 2 1 1 .2 0 .3 .4 .3 .7 0"},
 {"Warm Analog Pad","Pad","2 3 0 10 .5 0 3 .6 .2 1800 .25 .3 .6 1.5 .7 1.5 1 2 .25 .3 0 .5 .1 .5 .05"},
 {"Sweep Motion","Pad","2 2 7 14 .6 0 4 .9 0 900 .35 .6 1.5 2.5 .8 3 3 3.5 .12 .5 0 .6 .35 .7 0"},
 // ---- CHORDS
 {"Supersaw Stab","Chords","2 2 0 14 .8 0 5 1 0 4500 .2 .4 .003 .35 .5 .3 .003 .3 5 0 0 .4 .2 .35 .1"},
 {"Dream Organ","Chords","3 0 12 2 .5 0 2 .3 .5 5000 .1 0 .01 .1 .9 .15 .01 .1 6 0 .2 .3 0 .3 .1"},
 {"Lo-Fi Chord","Chords","1 0 0 6 .4 .2 1 0 .2 3000 .1 .3 .005 .8 .3 .5 .005 .6 4 0 .2 .5 .2 .4 .05"},
 {"Future House","Chords","2 3 12 10 .5 0 3 .6 0 3800 .25 .5 .003 .3 .4 .25 .003 .25 5 0 0 .3 .3 .3 .15"},
 {"Velvet Swell","Chords","2 2 0 9 .6 0 4 .7 .2 2200 .2 .4 .5 .8 .7 1 .6 1 .3 0 .1 .4 .25 .5 0"},
 // ---- PLUCK
 {"Crystal Pluck","Pluck","2 2 0 7 .4 0 2 .4 0 1500 .2 .8 .001 .25 0 .25 .001 .2 5 0 0 .3 .3 .35 0"},
 {"Kalimba","Pluck","0 0 12 0 .3 .4 1 0 0 8000 .1 .2 .001 .6 0 .5 .001 .4 5 0 0 .15 .3 .4 0"},
 {"Harp Pluck","Pluck","1 2 12 3 .3 0 1 0 0 3000 .1 .6 .001 1.2 0 .8 .001 .7 5 0 .1 .25 .2 .5 0"},
 {"Retro Pluck","Pluck","3 3 0 8 .5 0 2 .3 .2 1800 .3 .7 .001 .18 0 .15 .001 .15 5 0 0 .2 .25 .25 .1"},
 {"Marimba Wood","Pluck","0 0 12 0 .5 .6 1 0 .3 6000 .1 .1 .001 .35 0 .3 .001 .3 5 0 0 0 .1 .3 0"},
 // ---- BASS
 {"Sub Titan","Bass","0 1 0 0 .2 0 1 0 .6 900 .1 .2 .005 .3 1 .2 .005 .3 5 0 0 0 0 0 .2"},
 {"Reese Monster","Bass","2 2 0 18 .9 0 3 .6 .3 1100 .3 .3 .005 .4 .9 .3 .005 .4 .3 .4 0 .2 0 .1 .35"},
 {"Acid 303","Bass","2 2 0 0 0 0 1 0 0 600 .75 .85 .001 .2 .6 .1 .001 .18 5 0 0 0 .2 .1 .4"},
 {"FM Growl","Bass","0 0 0 0 .5 .7 1 0 .3 2500 .3 .3 .005 .3 .8 .2 .005 .3 3 .3 0 0 0 .1 .4"},
 {"Pluck Bass","Bass","3 2 0 4 .4 0 1 0 .4 800 .3 .6 .001 .25 .3 .12 .001 .2 5 0 0 0 0 .1 .1"},
 // ---- KEYS
 {"E-Piano Glow","Keys","0 0 12 0 .4 .35 1 0 0 5000 .1 .3 .002 1.2 .2 .6 .002 .8 5 0 .05 .45 .15 .3 0"},
 {"Dream Piano","Keys","1 0 12 4 .35 .2 1 0 .1 4000 .1 .4 .002 1.5 .1 .8 .002 1 5 0 0 .3 .2 .5 0"},
 {"Clav Funk","Keys","3 2 0 3 .5 0 1 0 0 1500 .35 .8 .001 .2 .2 .1 .001 .15 5 0 0 0 0 .1 .15"},
 {"Soft Organ","Keys","0 3 12 0 .4 .1 1 0 .5 3500 .1 0 .01 .1 .95 .1 .01 .1 6 0 .15 .3 0 .25 0"},
 {"Wurly Bite","Keys","0 1 0 5 .4 .5 1 0 0 3500 .15 .4 .002 .9 .3 .4 .002 .5 5.5 0 .15 .3 .15 .25 .25"},
 // ---- BELLS
 {"Ice Bell","Bells","0 0 19 0 .5 .8 1 0 0 12000 .1 0 .001 2.5 0 2.5 .001 1 5 0 0 .3 .3 .6 0"},
 {"Music Box","Bells","0 0 24 0 .3 .3 1 0 0 12000 .1 0 .001 1 0 1 .001 1 5 0 0 .1 .3 .5 0"},
 {"Gamelan Rain","Bells","0 0 7 0 .5 .9 1 0 0 9000 .1 .2 .001 1.8 0 1.8 .001 .8 5 0 0 .3 .4 .6 0"},
 {"Tubular","Bells","0 1 12 0 .4 .6 1 0 .2 7000 .1 0 .001 3 0 3 .001 1 4 0 .05 .2 .2 .6 0"},
 {"Celestial Chime","Bells","0 0 12 12 .5 .4 2 .6 0 12000 .1 0 .005 2 0 3 .005 1 5 0 .1 .5 .4 .8 0"},
 // ---- ARP
 {"Pulse Arp","Arp","3 2 0 6 .4 0 1 0 .2 2200 .3 .6 .001 .18 0 .12 .001 .15 5 0 0 .2 .35 .3 .1"},
 {"Cyber Arp","Arp","2 3 12 8 .5 0 2 .4 0 3000 .35 .7 .001 .2 .1 .12 .001 .2 5 0 0 .25 .35 .3 .15"},
 {"Sparkle Arp","Arp","0 2 12 5 .5 .3 1 0 0 8000 .2 .5 .001 .22 0 .2 .001 .2 5 0 0 .3 .4 .4 0"},
 {"Trance Gate","Arp","2 2 0 12 .7 0 4 .6 0 2500 .3 .5 .001 .2 .3 .1 .001 .15 8 .5 0 .2 .3 .3 .1"},
 {"Chip Hero","Arp","3 3 0 0 .5 0 1 0 0 12000 .1 0 .001 .15 .6 .1 .001 .1 5 0 .1 0 .2 .15 0"},
 // ---- ORCHESTRAL
 {"Epic Strings","Orchestral","2 2 0 9 .7 0 4 .7 0 3500 .15 .3 .6 .5 .9 .9 .5 1 5 0 .25 .35 0 .5 0"},
 {"Brass Fanfare","Orchestral","2 2 0 6 .6 0 3 .3 0 1500 .15 .7 .06 .3 .8 .25 .06 .4 5 0 .1 .1 0 .35 .1"},
 {"Cinematic Brass","Orchestral","2 3 -12 7 .6 0 3 .4 .3 1200 .2 .6 .15 .5 .8 .6 .15 .6 4 0 .1 .1 0 .5 .15"},
 {"Soft Flute","Orchestral","0 1 12 0 .2 .05 1 0 0 6000 .1 .2 .1 .3 .8 .3 .1 .3 5 0 .4 .1 0 .4 0"},
 {"Pizzicato","Orchestral","2 2 0 5 .5 0 2 .3 0 2000 .1 .6 .001 .22 0 .15 .001 .2 5 0 0 .1 0 .3 0"},
 // ---- ATMOS
 {"Cosmic Drift","Atmos","1 2 7 14 .6 .1 4 .9 0 1500 .3 .4 3 3 .8 5 3 3 .1 .6 .2 .6 .5 .9 0"},
 {"Frozen Lake","Atmos","0 0 12 10 .5 .3 3 .8 0 8000 .1 0 2 2 .9 4 2 2 .2 0 .3 .6 .5 .9 0"},
 {"Dark Forest","Atmos","2 4 -12 6 .15 0 3 .6 .4 600 .35 .3 3 3 .8 5 4 4 .08 .5 0 .4 .4 .8 .1"},
 {"Ghost Choir","Atmos","0 1 12 12 .6 .2 5 1 0 3500 .2 .2 1.5 2 .8 4 2 2 .15 .2 .4 .6 .4 .85 0"},
 {"Tidal","Atmos","2 2 0 10 .5 0 4 .8 .2 700 .45 .7 2 3 .7 4 3 4 .1 .7 0 .5 .35 .8 0"},
 // ---- FX
 {"Riser","FX","4 2 0 20 .5 0 3 1 0 400 .4 .9 4 .1 1 1 4 .1 .2 .3 0 .3 .4 .6 .1"},
 {"Laser Zap","FX","3 2 24 0 .5 .9 1 0 0 8000 .3 .6 .001 .25 0 .1 .001 .25 5 0 .8 0 .3 .3 .1"},
 {"Wind Whisper","FX","4 4 0 0 .5 0 1 0 0 1200 .6 .5 2 2 .8 3 3 3 .2 .6 0 .5 .3 .7 0"},
 {"Alien Wobble","FX","0 0 7 0 .5 .95 1 0 0 3000 .5 .3 .01 .5 .8 .5 .01 .5 7 .6 .6 .3 .3 .4 .2"},
 {"Static Burst","FX","4 3 0 0 .6 0 1 0 0 5000 .5 .2 .001 .3 0 .2 .001 .3 5 0 0 .2 .3 .3 .6"},
 // ---- PERC
 {"Synth Tom","Perc","0 4 -12 0 .2 0 1 0 .5 1500 .2 .6 .001 .2 0 .15 .001 .1 5 0 0 0 .1 .2 .2"},
 {"Wood Block","Perc","3 0 12 0 .3 .2 1 0 0 4000 .2 .5 .001 .06 0 .05 .001 .05 5 0 0 0 0 .2 0"},
 {"Steel Drum","Perc","0 0 12 0 .5 .5 1 0 0 6000 .1 .2 .001 .6 0 .5 .001 .5 5 0 .1 .3 .2 .4 0"},
 {"Hat Noise","Perc","4 4 0 0 .5 0 1 0 0 12000 .1 .5 .001 .05 0 .03 .001 .03 5 0 0 0 0 .1 0"},
 {"Pitched Kick","Perc","0 0 -24 0 0 0 1 0 .8 400 .2 .8 .001 .25 0 .1 .001 .08 5 0 0 0 0 .05 .3"},
};
static constexpr int kNP = (int) (sizeof(kPresets) / sizeof(kPresets[0]));
static const char* kCats[] = { "All", "Lead", "Pad", "Chords", "Pluck", "Bass", "Keys",
                               "Bells", "Arp", "Orchestral", "Atmos", "FX", "Perc" };
static constexpr int kNumCats = (int) (sizeof(kCats) / sizeof(kCats[0]));

// ----------------------------------------------------------------------------
//  Moteur audio
// ----------------------------------------------------------------------------
struct Env
{
    int st = 0; float v = 0.f;                       // 0 idle 1 att 2 dec 3 sus 4 rel
    void gate (bool on) { if (on) st = 1; else if (st != 0) st = 4; }
    float next (float a, float d, float s, float r, double sr)
    {
        switch (st)
        {
            case 1: v += 1.f / (jmax (a, 0.001f) * (float) sr); if (v >= 1.f) { v = 1.f; st = 2; } break;
            case 2: v += (s - v) * (1.f - std::exp (-3.f / (jmax (d, 0.005f) * (float) sr)));
                    if (std::abs (v - s) < 1e-4f) { v = s; st = 3; } break;
            case 3: v = s; if (s < 1e-4f) { v = 0.f; st = 0; } break;
            case 4: v -= v * (1.f - std::exp (-4.f / (jmax (r, 0.005f) * (float) sr)));
                    if (v < 1e-4f) { v = 0.f; st = 0; } break;
            default: v = 0.f; break;
        }
        return v;
    }
};

struct Voice
{
    bool on = false, held = false;
    int note = 0; float vel = 0.f; uint32 age = 0; double freq = 440.0;
    double ph1[5] = { 0, 0, 0, 0, 0 }, ph2 = 0, phs = 0;
    Env ae, fe; float ic1 = 0.f, ic2 = 0.f;
};

class AuroraProcessor : public AudioProcessor
{
public:
    AuroraProcessor()
        : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
          apvts (*this, nullptr, "STATE", createLayout())
    {
        for (int i = 0; i < kNumParams; ++i) raw[i] = apvts.getRawParameterValue (kParams[i].id);
        applyPreset (0);
    }

    static AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        AudioProcessorValueTreeState::ParameterLayout l;
        for (int i = 0; i < kNumParams; ++i)
        {
            auto& p = kParams[i];
            NormalisableRange<float> r (p.mn, p.mx, p.step);
            if (p.centre > 0.f) r.setSkewForCentre (p.centre);
            AudioParameterFloatAttributes a;
            if (i < 2) a = a.withStringFromValueFunction ([] (float v, int) { return String (kWaves[jlimit (0, 4, (int) std::round (v))]); });
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { p.id, 1 }, p.name, r, p.def, a));
        }
        return l;
    }

    // ---- presets
    void applyPreset (int i)
    {
        i = jlimit (0, kNP - 1, i); curPreset = i;
        auto t = StringArray::fromTokens (kPresets[i].v, " ", "");
        for (int k = 0; k < kPV && k < t.size(); ++k)
            if (auto* p = apvts.getParameter (kParams[k].id))
                p->setValueNotifyingHost (p->convertTo0to1 (t[k].getFloatValue()));
        apvts.state.setProperty ("preset", i, nullptr);
    }
    void mutate()
    {
        Random r;
        for (int k = 0; k < kPV; ++k)
        {
            if (k == 2) continue;
            auto* p = apvts.getParameter (kParams[k].id);
            if (k < 2) { if (r.nextFloat() < 0.2f) p->setValueNotifyingHost (r.nextFloat()); continue; }
            p->setValueNotifyingHost (jlimit (0.f, 1.f, p->getValue() + (r.nextFloat() - 0.5f) * 0.4f));
        }
    }
    void readScope (float* out, int n)
    {
        float tmp[1024]; int idx = scopeIdx.load();
        for (int i = 0; i < 1024; ++i) tmp[i] = scope[(idx + i) & 1023];
        int start = 0;
        for (int i = 1; i < 512; ++i) if (tmp[i - 1] < 0.f && tmp[i] >= 0.f) { start = i; break; }
        for (int i = 0; i < n; ++i) out[i] = tmp[start + i];
    }

    // ---- AudioProcessor
    const String getName() const override { return "Aurora Synth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return kNP; }
    int getCurrentProgram() override { return curPreset; }
    void setCurrentProgram (int i) override { if (i >= 0 && i < kNP) applyPreset (i); }
    const String getProgramName (int i) override { return (i >= 0 && i < kNP) ? String (kPresets[i].cat) + " - " + kPresets[i].name : String(); }
    void changeProgramName (int, const String&) override {}
    bool hasEditor() const override { return true; }
    AudioProcessorEditor* createEditor() override;
    bool isBusesLayoutSupported (const BusesLayout& l) const override { return l.getMainOutputChannelSet() == AudioChannelSet::stereo(); }

    void prepareToPlay (double sr, int maxBlock) override
    {
        sampleRate = sr;
        lfoBuf.assign ((size_t) jmax (maxBlock, 512), 0.f);
        dlyL.assign ((size_t) (sr * 3.0) + 2, 0.f); dlyR = dlyL; dw = 0;
        chL.assign (4096, 0.f); chR = chL; cw = 0;
        reverb.setSampleRate (sr); reverb.reset();
        for (auto& v : voices) v = Voice();
    }
    void releaseResources() override {}

    void getStateInformation (MemoryBlock& d) override
    {
        auto s = apvts.copyState(); s.setProperty ("preset", curPreset.load(), nullptr);
        if (auto xml = s.createXml()) copyXmlToBinary (*xml, d);
    }
    void setStateInformation (const void* d, int n) override
    {
        if (auto xml = getXmlFromBinary (d, n))
            if (xml->hasTagName (apvts.state.getType()))
            {
                apvts.replaceState (ValueTree::fromXml (*xml));
                curPreset = jlimit (0, kNP - 1, (int) apvts.state.getProperty ("preset", 0));
            }
    }

    void processBlock (AudioBuffer<float>& buf, MidiBuffer& midi) override
    {
        ScopedNoDenormals nd; buf.clear();
        float P[kNumParams]; for (int i = 0; i < kNumParams; ++i) P[i] = raw[i]->load();
        const int n = buf.getNumSamples();
        float* L = buf.getWritePointer (0);
        float* R = buf.getWritePointer (buf.getNumChannels() > 1 ? 1 : 0);
        if ((int) lfoBuf.size() < n) lfoBuf.resize ((size_t) n);
        for (int i = 0; i < n; ++i)
        {
            lfoBuf[(size_t) i] = (float) std::sin (MathConstants<double>::twoPi * lfoPh);
            lfoPh += P[18] / sampleRate; if (lfoPh >= 1.0) lfoPh -= 1.0;
        }

        int cursor = 0;
        for (const auto meta : midi)
        {
            int t = jlimit (cursor, n, meta.samplePosition);
            renderVoices (P, L, R, cursor, t); cursor = t;
            auto m = meta.getMessage();
            if (m.isNoteOn()) noteOn (m.getNoteNumber(), m.getFloatVelocity());
            else if (m.isNoteOff()) noteOff (m.getNoteNumber());
            else if (m.isPitchWheel()) bend = (m.getPitchWheelValue() - 8192) / 8192.f * 2.f;
            else if (m.isController())
            {
                if (m.getControllerNumber() == 1) modWheel = m.getControllerValue() / 127.f;
                if (m.getControllerNumber() == 64)
                {
                    sustain = m.getControllerValue() >= 64;
                    if (! sustain) for (auto& v : voices) if (v.held) { v.held = false; v.ae.gate (false); v.fe.gate (false); }
                }
            }
            else if (m.isAllNotesOff() || m.isAllSoundOff()) for (auto& v : voices) v.ae.gate (false);
        }
        renderVoices (P, L, R, cursor, n);

        // ---- effets : drive -> chorus -> delay ping-pong (1/8 pointée) -> reverb
        double bpm = 120.0;
        if (auto* ph = getPlayHead()) if (auto info = ph->getPosition()) if (auto b = info->getBpm()) bpm = *b;
        const int dSamp = jlimit (64, (int) dlyL.size() - 1, (int) (60.0 / jmax (30.0, bpm) * 0.75 * sampleRate));
        const float dg = 0.8f + P[24] * 5.f, chor = P[21], dmix = P[22] * 0.8f;
        const int csz = (int) chL.size(), dsz = (int) dlyL.size();
        auto rd = [&] (std::vector<float>& b, float d)
        {
            float rp = (float) cw - d; if (rp < 0.f) rp += (float) csz;
            int i0 = (int) rp; float fr = rp - (float) i0;
            return b[(size_t) i0] * (1.f - fr) + b[(size_t) ((i0 + 1) % csz)] * fr;
        };
        for (int i = 0; i < n; ++i)
        {
            float l = std::tanh (L[i] * dg), r = std::tanh (R[i] * dg);
            chL[(size_t) cw] = l; chR[(size_t) cw] = r;
            chPh += 0.35 / sampleRate; if (chPh >= 1.0) chPh -= 1.0;
            float m1 = (float) std::sin (MathConstants<double>::twoPi * chPh);
            float m2 = (float) std::sin (MathConstants<double>::twoPi * (chPh + 0.25));
            float cl = rd (chL, (0.012f + 0.006f * m1) * (float) sampleRate);
            float cr = rd (chR, (0.012f + 0.006f * m2) * (float) sampleRate);
            l = l * (1.f - 0.3f * chor) + cl * chor * 0.7f;
            r = r * (1.f - 0.3f * chor) + cr * chor * 0.7f;
            cw = (cw + 1) % csz;
            int rp = (dw - dSamp + dsz) % dsz;
            float dl = dlyL[(size_t) rp], dr = dlyR[(size_t) rp];
            dlyL[(size_t) dw] = l + dr * 0.45f; dlyR[(size_t) dw] = r + dl * 0.45f;
            l += dl * dmix; r += dr * dmix; dw = (dw + 1) % dsz;
            L[i] = l; R[i] = r;
        }
        Reverb::Parameters rp; rp.roomSize = 0.8f; rp.damping = 0.45f; rp.width = 1.f; rp.freezeMode = 0.f;
        rp.wetLevel = P[23] * 0.4f; rp.dryLevel = 1.f - P[23] * 0.2f; reverb.setParameters (rp);
        if (buf.getNumChannels() > 1) reverb.processStereo (L, R, n);
        const float vol = P[25] * 1.3f;
        for (int i = 0; i < n; ++i)
        {
            L[i] = jlimit (-1.f, 1.f, L[i] * vol); R[i] = jlimit (-1.f, 1.f, R[i] * vol);
            if (++dec >= 4) { dec = 0; scope[(size_t) (scopeIdx.load() & 1023)] = 0.5f * (L[i] + R[i]); scopeIdx++; }
        }
    }

    AudioProcessorValueTreeState apvts;
    std::atomic<int> curPreset { 0 };

private:
    std::atomic<float>* raw[kNumParams] {};
    Voice voices[16]; uint32 counter = 0;
    double sampleRate = 44100.0, lfoPh = 0.0, chPh = 0.0;
    float bend = 0.f, modWheel = 0.f; bool sustain = false;
    std::vector<float> lfoBuf, dlyL, dlyR, chL, chR; int dw = 0, cw = 0;
    Reverb reverb; Random rng;
    float scope[1024] {}; std::atomic<int> scopeIdx { 0 }; int dec = 0;

    void noteOn (int n, float vel)
    {
        Voice* t = nullptr;
        for (auto& v : voices) if (! v.on) { t = &v; break; }
        if (! t) { t = &voices[0]; for (auto& v : voices) if (v.age < t->age) t = &v; }
        const bool uni = raw[6]->load() > 1.5f;
        *t = Voice(); t->on = true; t->note = n; t->vel = 0.35f + 0.65f * vel; t->age = ++counter;
        t->freq = 440.0 * std::pow (2.0, (n - 69) / 12.0);
        if (uni) { for (auto& p : t->ph1) p = rng.nextDouble(); t->ph2 = rng.nextDouble(); }
        t->ae.gate (true); t->fe.gate (true);
    }
    void noteOff (int n)
    {
        for (auto& v : voices)
            if (v.on && v.note == n && v.ae.st != 4)
            { if (sustain) v.held = true; else { v.ae.gate (false); v.fe.gate (false); } }
    }
    static float blep (double t, double dt)
    {
        if (t < dt) { t /= dt; return (float) (t + t - t * t - 1.0); }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float) (t * t + t + t + 1.0); }
        return 0.f;
    }
    float osc (int w, double t, double dt)
    {
        switch (w)
        {
            case 0: return (float) std::sin (MathConstants<double>::twoPi * t);
            case 1: return (float) (4.0 * std::abs (t - 0.5) - 1.0);
            case 2: return (float) (2.0 * t - 1.0) - blep (t, dt);
            case 3: { double t2 = t + 0.5; t2 -= std::floor (t2);
                      return (t < 0.5 ? 1.f : -1.f) + blep (t, dt) - blep (t2, dt); }
            default: return rng.nextFloat() * 2.f - 1.f;
        }
    }
    void renderVoices (const float* P, float* L, float* R, int s, int e)
    {
        if (s >= e) return;
        const int uni = jlimit (1, 5, (int) std::round (P[6]));
        const int w1 = jlimit (0, 4, (int) std::round (P[0])), w2 = jlimit (0, 4, (int) std::round (P[1]));
        float ratio[5];
        for (int u = 0; u < 5; ++u)
            ratio[u] = std::exp2 ((uni > 1 ? ((float) u / (float) (uni - 1) * 2.f - 1.f) * P[7] * 0.35f : 0.f) / 12.f);
        const double r2 = std::exp2 (P[2] / 12.0 + P[3] / 1200.0);
        const float ug = 1.f / std::sqrt ((float) uni);
        const double isr = 1.0 / sampleRate, tp = MathConstants<double>::twoPi;
        for (auto& v : voices)
        {
            if (! v.on) continue;
            for (int i = s; i < e; ++i)
            {
                const float lfo = lfoBuf[(size_t) i];
                const double f0 = v.freq * std::exp2 (((double) bend + (P[20] + modWheel) * lfo * 0.6) / 12.0);
                const double inc0 = f0 * isr;
                const double fm = P[5] > 0.001f ? P[5] * 0.8 * std::sin (tp * v.ph2) : 0.0;
                float s1 = 0.f;
                for (int u = 0; u < uni; ++u)
                {
                    double inc = inc0 * ratio[u], t = v.ph1[u] + fm; t -= std::floor (t);
                    s1 += osc (w1, t, inc);
                    v.ph1[u] += inc; if (v.ph1[u] >= 1.0) v.ph1[u] -= 1.0;
                }
                const double inc2 = inc0 * r2;
                const float s2 = osc (w2, v.ph2, inc2);
                v.ph2 += inc2; if (v.ph2 >= 1.0) v.ph2 -= 1.0;
                const float sub = P[8] > 0.001f ? (float) std::sin (tp * v.phs) : 0.f;
                v.phs += inc0 * 0.5; if (v.phs >= 1.0) v.phs -= 1.0;
                const float x = s1 * ug + s2 * P[4] + sub * P[8];

                const float ae = v.ae.next (P[12], P[13], P[14], P[15], sampleRate);
                if (v.ae.st == 0) { v.on = false; break; }
                const float fe = v.fe.next (P[16], P[17], 0.f, P[17], sampleRate);
                double cut = P[9] * std::exp2 (P[11] * fe * 5.f + P[19] * lfo * 2.f + (v.note - 60) / 24.f);
                cut = jlimit (20.0, jmin (18000.0, sampleRate * 0.45), cut);
                const float g = (float) std::tan (MathConstants<double>::pi * cut * isr);
                const float k = 2.f - 2.f * P[10];
                const float a1 = 1.f / (1.f + g * (g + k)), a2 = g * a1, a3 = g * a2;
                const float v3 = x - v.ic2, v1 = a1 * v.ic1 + a2 * v3, v2 = v.ic2 + a2 * v.ic1 + a3 * v3;
                v.ic1 = 2.f * v1 - v.ic1; v.ic2 = 2.f * v2 - v.ic2;
                const float o = v2 * ae * v.vel * 0.3f;
                L[i] += o; R[i] += o;
            }
        }
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuroraProcessor)
};

// ----------------------------------------------------------------------------
//  Interface futuriste
// ----------------------------------------------------------------------------
static const Colour kCyan (0xff00e5ff), kMagenta (0xffff3df2), kTxt (0xffa9b6e0);

struct AuroraLAF : LookAndFeel_V4
{
    void drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider&) override
    {
        auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (5.f);
        const float r = jmin (b.getWidth(), b.getHeight()) / 2.f; auto c = b.getCentre();
        const float ang = a0 + pos * (a1 - a0);
        Path track; track.addCentredArc (c.x, c.y, r, r, 0.f, a0, a1, true);
        g.setColour (Colour (0x30ffffff));
        g.strokePath (track, PathStrokeType (3.f, PathStrokeType::curved, PathStrokeType::rounded));
        Path val; val.addCentredArc (c.x, c.y, r, r, 0.f, a0, ang, true);
        auto col = kCyan.interpolatedWith (kMagenta, pos);
        g.setColour (col.withAlpha (0.25f));
        g.strokePath (val, PathStrokeType (8.f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (col);
        g.strokePath (val, PathStrokeType (3.f, PathStrokeType::curved, PathStrokeType::rounded));
        const float rb = r * 0.68f;
        g.setGradientFill (ColourGradient (Colour (0xff2a2f4a), c.x, c.y - rb, Colour (0xff0b0d1a), c.x, c.y + rb, false));
        g.fillEllipse (c.x - rb, c.y - rb, rb * 2.f, rb * 2.f);
        Path p; p.addRoundedRectangle (-1.5f, -rb, 3.f, rb * 0.55f, 1.5f);
        g.setColour (col);
        g.fillPath (p, AffineTransform::rotation (ang).translated (c.x, c.y));
    }
    void drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.f);
        g.setColour (kCyan.withAlpha (down ? 0.35f : over ? 0.22f : 0.10f)); g.fillRoundedRectangle (r, 8.f);
        g.setColour (kCyan.withAlpha (0.8f)); g.drawRoundedRectangle (r, 8.f, 1.2f);
    }
};

struct SimpleList : ListBox, ListBoxModel
{
    StringArray items; std::function<void (int)> onSelect;
    SimpleList() { setModel (this); setRowHeight (25); setOutlineThickness (0);
                   setColour (backgroundColourId, Colours::transparentBlack); }
    int getNumRows() override { return items.size(); }
    void paintListBoxItem (int row, Graphics& g, int w, int h, bool sel) override
    {
        if (row >= items.size()) return;
        if (sel)
        {
            g.setGradientFill (ColourGradient (kCyan.withAlpha (0.35f), 0.f, 0.f, kMagenta.withAlpha (0.35f), (float) w, 0.f, false));
            g.fillRoundedRectangle (2.f, 1.f, (float) (w - 4), (float) (h - 2), 6.f);
        }
        g.setColour (sel ? Colours::white : kTxt); g.setFont (Font (FontOptions (13.f)));
        g.drawText (items[row], 10, 0, w - 12, h, Justification::centredLeft);
    }
    void selectedRowsChanged (int r) override { if (onSelect && r >= 0) onSelect (r); }
    void set (const StringArray& s) { items = s; updateContent(); repaint(); }
};

class AuroraEditor : public AudioProcessorEditor, private Timer
{
public:
    explicit AuroraEditor (AuroraProcessor& p) : AudioProcessorEditor (&p), proc (p)
    {
        setLookAndFeel (&laf); setSize (940, 580);
        for (int i = 0; i < kNumParams; ++i)
        {
            auto& k = knobs[i];
            k.s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
            k.s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
            k.s.setPopupDisplayEnabled (true, true, this);
            k.s.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
            k.l.setText (kParams[i].name, dontSendNotification);
            k.l.setJustificationType (Justification::centred);
            k.l.setFont (Font (FontOptions (11.f))); k.l.setColour (Label::textColourId, kTxt);
            k.a = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, kParams[i].id, k.s);
            addAndMakeVisible (k.s); addAndMakeVisible (k.l);
        }
        StringArray cn; for (auto* c : kCats) cn.add (c);
        cats.set (cn); addAndMakeVisible (cats); addAndMakeVisible (presets);
        cats.onSelect = [this] (int r) { if (! silent) { cat = r; refresh(); } };
        presets.onSelect = [this] (int r)
        { if (! silent && r < (int) ids.size()) { proc.applyPreset (ids[(size_t) r]); lastPreset = proc.curPreset; repaint(); } };
        for (auto* b : { &prev, &next, &mutateBtn }) addAndMakeVisible (b);
        prev.setButtonText ("<"); next.setButtonText (">"); mutateBtn.setButtonText ("MUTATE");
        prev.onClick = [this] { step (-1); }; next.onClick = [this] { step (1); };
        mutateBtn.onClick = [this] { proc.mutate(); };
        lastPreset = proc.curPreset; refresh();
        startTimerHz (30);
    }
    ~AuroraEditor() override { setLookAndFeel (nullptr); }

    void paint (Graphics& g) override
    {
        const float t = (float) (Time::getMillisecondCounterHiRes() * 0.001);
        const float W = (float) getWidth(), H = (float) getHeight();
        g.fillAll (Colour (0xff060814));
        auto blob = [&] (float x, float y, float r, Colour c)
        { g.setGradientFill (ColourGradient (c.withAlpha (0.20f), x, y, c.withAlpha (0.f), x + r, y, true)); g.fillRect (getLocalBounds()); };
        blob (W * (0.25f + 0.15f * std::sin (t * 0.30f)), H * (0.20f + 0.10f * std::cos (t * 0.25f)), 460.f, kCyan);
        blob (W * (0.80f + 0.10f * std::cos (t * 0.22f)), H * (0.30f + 0.15f * std::sin (t * 0.31f)), 440.f, kMagenta);
        blob (W * (0.55f + 0.20f * std::sin (t * 0.17f)), H * (0.95f + 0.05f * std::cos (t * 0.4f)), 520.f, Colour (0xff6a3dff));

        g.setColour (Colours::white); g.setFont (Font (FontOptions (32.f, Font::bold)));
        g.drawText ("AURORA", 18, 8, 200, 40, Justification::centredLeft);
        g.setColour (kCyan); g.setFont (Font (FontOptions (11.f, Font::bold)));
        g.drawText ("S Y N T H", 20, 44, 200, 16, Justification::centredLeft);
        const int cp = proc.curPreset;
        g.setColour (kCyan); g.setFont (Font (FontOptions (20.f, Font::bold)));
        g.drawText (String (kPresets[cp].cat).toUpperCase() + "  /  " + kPresets[cp].name, 300, 14, 625, 34, Justification::centredRight);

        for (auto& s : secs)
        {
            auto r = s.second.toFloat();
            g.setColour (Colour (0x12ffffff)); g.fillRoundedRectangle (r, 10.f);
            g.setColour (kCyan.withAlpha (0.18f)); g.drawRoundedRectangle (r, 10.f, 1.f);
            g.setColour (Colour (0xff7d8bc4)); g.setFont (Font (FontOptions (10.f, Font::bold)));
            g.drawText (s.first, s.second.getX() + 8, s.second.getY() + 4, s.second.getWidth() - 8, 14, Justification::centredLeft);
        }
        // oscilloscope
        g.setColour (Colour (0x12ffffff)); g.fillRoundedRectangle (scopeR.toFloat(), 10.f);
        g.setColour (kCyan.withAlpha (0.18f)); g.drawRoundedRectangle (scopeR.toFloat(), 10.f, 1.f);
        float d[512]; proc.readScope (d, 512);
        Path p; auto sr = scopeR.toFloat().reduced (10.f, 8.f);
        for (int i = 0; i < 512; ++i)
        {
            float px = sr.getX() + sr.getWidth() * (float) i / 511.f;
            float py = sr.getCentreY() - jlimit (-1.f, 1.f, d[i] * 1.5f) * sr.getHeight() * 0.5f;
            if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
        }
        g.setColour (kMagenta.withAlpha (0.25f)); g.strokePath (p, PathStrokeType (6.f, PathStrokeType::curved));
        g.setColour (kCyan); g.strokePath (p, PathStrokeType (1.8f, PathStrokeType::curved));
    }

    void resized() override
    {
        struct Sec { const char* title; std::vector<int> ids; };
        const std::vector<std::vector<Sec>> rows = {
            { { "OSCILLATORS", { 0, 1, 2, 3, 4, 5, 6, 7, 8 } } },
            { { "FILTER", { 9, 10, 11 } }, { "FILTER ENV", { 16, 17 } }, { "AMP ENV", { 12, 13, 14, 15 } } },
            { { "MODULATION", { 18, 19, 20 } }, { "EFFECTS", { 21, 22, 23, 24 } }, { "OUT", { 25 } } } };
        secs.clear(); int y = 78;
        for (auto& row : rows)
        {
            int x = 300;
            for (auto& s : row)
            {
                int w = (int) s.ids.size() * 66;
                secs.push_back ({ s.title, Rectangle<int> (x, y, w, 104) });
                for (size_t j = 0; j < s.ids.size(); ++j)
                {
                    auto& k = knobs[s.ids[j]]; int kx = x + (int) j * 66;
                    k.s.setBounds (kx + 4, y + 20, 58, 58); k.l.setBounds (kx, y + 80, 66, 16);
                }
                x += w + 8;
            }
            y += 112;
        }
        scopeR = { 300, 420, 620, 148 };
        cats.setBounds (12, 78, 130, 330); presets.setBounds (150, 78, 140, 490);
        prev.setBounds (12, 424, 62, 30); next.setBounds (80, 424, 62, 30); mutateBtn.setBounds (12, 462, 130, 34);
    }

private:
    struct Knob { Slider s; Label l; std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> a; };
    AuroraProcessor& proc; AuroraLAF laf;
    Knob knobs[kNumParams]; SimpleList cats, presets; TextButton prev, next, mutateBtn;
    std::vector<std::pair<String, Rectangle<int>>> secs; Rectangle<int> scopeR;
    std::vector<int> ids; int cat = 0, lastPreset = 0; bool silent = false;

    void timerCallback() override
    { if (proc.curPreset != lastPreset) { lastPreset = proc.curPreset; refresh(); } repaint(); }

    void refresh()
    {
        silent = true; ids.clear(); StringArray n;
        for (int i = 0; i < kNP; ++i)
            if (cat == 0 || String (kPresets[i].cat) == kCats[cat]) { ids.push_back (i); n.add (kPresets[i].name); }
        presets.set (n); int row = -1;
        for (size_t j = 0; j < ids.size(); ++j) if (ids[j] == proc.curPreset) row = (int) j;
        if (row >= 0) presets.selectRow (row, false, true); else presets.deselectAllRows();
        silent = false;
    }
    void step (int d)
    {
        if (ids.empty()) return;
        int pos = 0; for (size_t j = 0; j < ids.size(); ++j) if (ids[j] == proc.curPreset) pos = (int) j;
        int np = (pos + d + (int) ids.size()) % (int) ids.size();
        proc.applyPreset (ids[(size_t) np]); lastPreset = proc.curPreset; refresh();
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuroraEditor)
};

AudioProcessorEditor* AuroraProcessor::createEditor() { return new AuroraEditor (*this); }
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AuroraProcessor(); }

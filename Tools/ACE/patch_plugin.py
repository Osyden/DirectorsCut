"""Apply the two source compatibility fixes observed building ACE 2.5 with UE 5.8.

Run only against this isolated checkout's vendor plugin. Original ZIP is retained.
"""
from pathlib import Path
import json

root = Path(__file__).resolve().parents[2] / "Plugins/NV_ACE_Reference"
descriptor = root / "NV_ACE_Reference.uplugin"
data = json.loads(descriptor.read_text(encoding="utf-8-sig"))
data.pop("EngineVersion", None)  # Rebuild from source, rather than claiming vendor UE 5.8 support.
data["Installed"] = False
descriptor.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
model_descriptor = root.parent / "NvAudio2FaceMarkRegressive/NvAudio2FaceMarkRegressive.uplugin"
if model_descriptor.exists():
    model_data = json.loads(model_descriptor.read_text(encoding="utf-8-sig"))
    model_data.pop("EngineVersion", None)
    model_data["Installed"] = False
    model_descriptor.write_text(json.dumps(model_data, indent=2) + "\n", encoding="utf-8")

path = root / "Source/ACECore/Private/ACECoreModule.cpp"
source = path.read_text(encoding="utf-8-sig")
old = "FConfigSection* NewSection = ConfigFile->FindOrAddSection(NewSectionName);"
new = """// UE 5.8 removed mutable FindOrAddSection. Copy before inserting to avoid map invalidation.
            const FConfigSection OldEntries = OldSection ? *OldSection : FConfigSection();
            const FConfigSection* ExistingSection = ConfigFile->FindSection(NewSectionName);
            FConfigSection MergedSection = ExistingSection ? *ExistingSection : FConfigSection();
            FConfigSection* NewSection = &ConfigFile->Add(NewSectionName, MoveTemp(MergedSection));"""
if old in source:
    source = source.replace(old, new).replace("Entry : *OldSection", "Entry : OldEntries")
    path.write_text(source, encoding="utf-8")
elif "const FConfigSection OldEntries" not in source:
    raise RuntimeError("Unexpected ACECore source; review the vendor version before patching")

path = root / "Source/OmniverseLiveLink/Private/OmniverseLiveLinkListener.cpp"
source = path.read_text(encoding="utf-8-sig")
old = "for (TPair<FString, TSharedPtr<FJsonValue>>& JsonField : JsonObject->Values)"
new = "for (const auto& JsonField : JsonObject->Values)"
if old in source:
    path.write_text(source.replace(old, new), encoding="utf-8")
elif new not in source:
    raise RuntimeError("Unexpected LiveLink source; review the vendor version before patching")
print("Applied ACE 2.5 UE 5.8 config-map and JSON iterator compatibility fixes")

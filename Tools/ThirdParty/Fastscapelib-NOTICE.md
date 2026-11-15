# Fastscapelib upstream reproduction notice

`Tools/run_fastscape_escarpment_reference.py` is an offline, source-only reproduction wrapper. It loads an external isolated Fastscapelib installation and executes the unchanged computational cells of an explicitly supplied upstream notebook. No Fastscapelib solver source, binaries or notebook cells are vendored into the game or this wrapper. It is not an Unreal runtime dependency.

- Upstream: [Fastscapelib, by the Fastscapelib developers](https://github.com/fastscape-lem/fastscapelib).
- Pinned package: `fastscapelib==0.3.0`.
- Matching published source revision: `707fcc10976b0e729f1415fd16b0a161e1f746c3`.
- Official example: [`doc/source/examples/escarpment_py.ipynb`](https://github.com/fastscape-lem/fastscapelib/blob/707fcc10976b0e729f1415fd16b0a161e1f746c3/doc/source/examples/escarpment_py.ipynb).
- Notebook SHA256: `4df4a91a2560d3c821c1e8841695b0a229a1cd7e367b7c110ba81b58b039da49`.
- Upstream licence: [GNU GPL version 3, full pinned source text](https://github.com/fastscape-lem/fastscapelib/blob/707fcc10976b0e729f1415fd16b0a161e1f746c3/LICENSE).
- Licence file SHA256: `8ceb4b9ee5adedde47b31e975c1d90c73ad27b6b165a1dcd80c7c545eb65b903`.

The full upstream licence, exact notebook and attribution are retained with every generated Saved reproduction bundle as `LICENSE.fastscapelib`, `upstream_escarpment_py.ipynb` and `ATTRIBUTION.txt`. Keep them together when sharing that bundle. Any later copying, modifying or redistribution of upstream code must retain its applicable notices and licence; this notice does not relicense upstream code or approve embedding it in Unreal.

The isolated dependency path used for the measured run was `Saved/FastscapeReference/python_packages`. Blender's and system Python's installed packages were not changed. The wrapper has no network or installation behavior; source downloads and the explicitly pinned isolated installation are separate setup steps documented in the handoff.

`Tools/generate_fastscape_crater_sector.py` is a separate offline API adaptation using that same isolated package. It generates a new physical terrain rather than executing the official notebook unchanged. Its optional valid-edge diffusion is project code replacing the grid-only ADI operation, which failed the irregular-domain boundary checks. Each new output bundle retains the exact generator source, the external package's full licence and attribution. This does not change the upstream licence or make the solver an Unreal runtime dependency. See [physical-sector handoff](../../taskboard/FASTSCAPE-PHYSICAL-SECTOR-15NOV.md) for the two rejected visual trials and method limitations.

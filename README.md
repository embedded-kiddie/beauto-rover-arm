# beauto-rover-arm
ビュートローバーARMのライントレース・ソフトウェア - ハーフスクラッチ開発の勧め

## 本リポジトリの目的
Vstone社製 [ビュートローバーARM] に付属のライブラリ `vs-wrc103.c` を使わず、[LPC1343] のCPU依存部分を定義した `CMSIS_CORE_LPC13xx` を元にライントレース用のコードベースをハーフスクラッチ開発したところ、とても勉強になったので、ここに情報を共有します。

本リポジトリはソースコードが流用されることを目的としてはいません。流用するなら [microbuilder/LPC1343CodeBase] をお勧めします。ここではビュートローバーARMを手にした人たち - おそらくはプログラミングの初学者やロボットに興味のあるホビーストたち - を対象に、電子工作としての組み込みプログラミングの楽しさを知ってもらうことを目的としています。

[ビュートローバーARM]: https://www.vstone.co.jp/products/beauto_rover/index.html "Beauto Rover H8/ARM（ビュートローバー） | ヴイストン株式会社"

[LPC1343]: https://www.nxp.jp/products/processors-and-microcontrollers/arm-microcontrollers/general-purpose-mcus/lpc1300-arm-cortex-m3/entry-level-32-bit-microcontroller-mcu-based-on-arm-cortex-m3-core:LPC1343FBD48 "LPC1343FBD48|Arm Cortex-M3|32-bit MCU | NXP Semiconductors"

[microbuilder/LPC1343CodeBase]: https://github.com/microbuilder/LPC1343CodeBase "Generic GCC-based library for the ARM Cortex-M3 LPC1343"

## ハーフスクラッチ開発の背景
[ビュートローバーARM][ビュートローバー仕様] には、センサ入力やモーター制御を行うライブラリ関数が用意されていて、C言語の初学者が遊びながらプログラミングを学ぶには良い素材だと思います。A/D変換やPWM（Pulse Width Modulation）の詳細を知らなくても、ビュートローバーを走らせることはできるでしょう。

[ビュートローバー仕様]: https://www.vstone.co.jp/products/beauto_rover/spec.html "ビュートローバーARM/H8 (Beauto Rover) - 製品仕様詳細| ヴイストン株式会社"

ですがさらに一歩踏み込んで「A/D変換のバーストモード」や「割り込み」、「タイマー」、「ウォッチドッグ」、「スリープモードやディープスリープモード」などCPUの持つ機能を学習すれば、組み込みプログラミングの基礎を習得できると共に、電子工作の幅を広げられる思います。

[Vstone社のギャラリー] にはビュートローバーの応用例が紹介されていますが、[ARM Cortex-M3&reg;]「[LPC1343]」搭載のCPUボード [VS-WRC103LV] は、単三電池２本で動くクロック72MHzのCPUボードとして、流行りの Arduino Uno などにも劣らない素材ではないかと思っています。

[Vstone社のギャラリー]: https://vstone1806.sakura.ne.jp/products/beauto_rover/gallery.html "ビュートローバーARM/H8 (Beauto Rover) - ギャラリー | ヴイストン株式会社"

[ARM Cortex-M3&reg;]: https://www.arm.com/ja/products/silicon-ip-cpu/cortex-m/cortex-m3 "Cortex-M3 – Arm&reg;"

[VS-WRC103LV]: https://www.vstone.co.jp/products/vs_wrc103lv/index.html "ARMマイコン搭載CPUボード「VS-WRC103LV」 | ヴイストン株式会社"

このCPUを使いこなすには、[LPC1343ユーザ・マニュアル]（アカウント登録とサインインが必要です）の読み込みが必須です。本リポジトリに上げたコードにはデータシートへの参照を記してあるので、CPUレジスタの使い方を習得する一助としてもらえればと思います。

本リポジトリに上げた各モジュールのコードには動作例を残してあります。これはソフトウェアのテストコードというより、むしろハードウェアの動作を1つ1つ実験しながら確認するためのコードとしています。例えば「PWMとは」を一般論として学ぶだけでなく「CPUレジスタをこう使えばデューティー比が変えられる」ことを学べばより理解が深まりますし、赤外線センサの出力特性を可視化してみればデバイスに関する知見が増えることでしょう。

またライントレースとは無関係ですが、消費電力を大幅に抑えながら一定周期ごとや外部トリガによって起動するディープスリープモードやウォッチドッグの基本などを（そのうち電圧低下検知機能も…）動作例を通して確認することができます。

ただ残念なことに [LPC1343] はリリースが2009年（[Wikipedia] 及び [LPC1343ユーザ・マニュアル] のリビジョン履歴による）と古く、LPC-linkといった [ICE（In-Circuit Emulator）] が絶版で入手不能となっているため、[VS-WRC103LV] ではソースコードデバッグが出来ません。本リポジトリでは、せめて `printf` デバッグできるようモジュールを作成しています。

[LPC1343ユーザ・マニュアル]: https://www.nxp.jp/webapp/Download?colCode=UM10375&lang_cd=ja "LPC1311/13/42/43 User manual"

[Wikipedia]: https://en.wikipedia.org/wiki/NXP_LPC "NXP LPC - Wikipedia"

[ICE（In-Circuit Emulator）]: https://ja.wikipedia.org/wiki/%E3%82%A4%E3%83%B3%E3%82%B5%E3%83%BC%E3%82%AD%E3%83%83%E3%83%88%E3%83%BB%E3%82%A8%E3%83%9F%E3%83%A5%E3%83%AC%E3%83%BC%E3%82%BF "インサーキット・エミュレータ - Wikipedia"

コードベースの開発に当たっては、抽象化の方針を決める必要があります。言うまでもなくコードベースの役割は、ハードウェアのレイヤーを抽象化しアプリケーション構築をし易くすることです。そのためには、どのようにモジュールを切り分けその役割を定義するか、何を隠蔽し何を公開するか、アプリケーションとのI/Fをどこで切るか等々を考えなくてはなりません。

例えばハードウェアの持つ機能単位でモジュールを切り分けた場合、必ずしも構築したいアプリケーションにとって最適とは言えないかもしれません。先に挙げた [microbuilder/LPC1343CodeBase] では [GPIO（General Purpose I/O）] へのアクセスは抽象化されていますが、スイッチが押されたかどうかを調べるにはさらに、[ポーリング] するか割り込みを使うか、[チャタリング] はハードウェアで除去されているのか、ソフトウェアで処理すべきか、等々を考慮する必要があります。一方アプリケーションのレイヤーではそのようなハードウェアへの依存を持たせたくないとすれば、もう１階層レイヤーを増やす必要があります。同コードベースではこれを `project` というレイヤーに集約しています。

本リポジトリに上げたコードベースでは（階層を増やしたくなかったので…）そこまではせず、「VS-WRC103LVのライントレース用」に特化させています。いずれの場合でも、ハーフスクラッチ開発はフルスクラッチよりハードルが低く、また自分の目的と目標に合わせた開発が可能で、かつ学習効果も高いのがお勧めする理由です。

[GPIO（General Purpose I/O）]: https://ja.wikipedia.org/wiki/gpio "gpio - Wikipedia"

[チャタリング]: https://ja.wikipedia.org/wiki/%E3%83%81%E3%83%A3%E3%82%BF%E3%83%AA%E3%83%B3%E3%82%B0 "チャタリング - Wikipedia"

[ポーリング]: https://ja.wikipedia.org/wiki/%E3%83%9D%E3%83%BC%E3%83%AA%E3%83%B3%E3%82%B0_(%E6%83%85%E5%A0%B1) "ポーリング (情報) - Wikipedia"

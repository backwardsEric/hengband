#include "mind/mind-explanations-table.h"

/*! 特殊技能の一覧テーブル */
const std::vector<mind_power> mind_powers = {
    { {
        /* Level gained,  cost,  %fail,  name */
        { 1, 1, 15, _("霊視", "Precognition") },
        { 2, 1, 20, _("神経攻撃", "Neural Blast") },
        { 3, 2, 25, _("次元の瞬き", "Minor Displacement") },
        { 7, 6, 35, _("虚空の幻影", "Major Displacement") },
        { 9, 7, 50, _("精神支配", "Domination") },

        { 11, 7, 30, _("念動衝撃弾", "Pulverise") },
        { 13, 12, 50, _("鎧化", "Character Armour") },
        { 15, 12, 60, _("サイコメトリー", "Psychometry") },
        { 18, 10, 45, _("精神波動", "Mind Wave") },
        { 23, 15, 50, _("アドレナリン・ドーピング", "Adrenaline Channeling") },

        { 26, 28, 60, _("テレキネシス", "Telekinesis") },
        { 28, 10, 40, _("サイキック・ドレイン", "Psychic Drain") },
        { 35, 35, 75, _("光の剣", "Psycho-Spear") },
        { 45, 150, 85, _("完全な世界", "The World") },
    } },

    { {
        /* Level gained,  cost,  %fail,  name */
        { 1, 1, 15, _("小龍", "Small Force Ball") },
        { 3, 3, 30, _("閃光", "Flash Light") },
        { 5, 6, 35, _("舞空術", "Flying Technique") },
        { 8, 5, 40, _("カメハメ波", "Kamehameha") },
        { 10, 7, 45, _("対魔法防御", "Magic Resistance") },

        { 13, 5, 60, _("練気", "Improve Force") },
        { 17, 17, 50, _("纏闘気", "Aura of Force") },
        { 20, 20, 50, _("衝波", "Shock Power") },
        { 23, 18, 55, _("彗龍", "Large Force Ball") },
        { 25, 30, 70, _("いてつく波動", "Dispel Magic") },

        { 28, 26, 50, _("幻霊召喚", "Summon Ghost") },
        { 32, 35, 65, _("爆発波", "Exploding Flame") },
        { 38, 42, 75, _("超カメハメ波", "Super Kamehameha") },
        { 44, 50, 80, _("光速移動", "Light Speed") },
    } },

    { {
        /* Level gained,  cost,  %fail,  name */
        { 8, 5, 40, _("殺気感知", "Detect Atmosphere of Menace") },
        { 15, 20, 0, _("突撃", "Charge") },
        { 20, 15, 0, _("トラップ粉砕", "Smash a Trap") },
        { 25, 20, 60, _("地震", "Quake") },
        { 30, 80, 75, _("皆殺し", "Massacre") },
    } },

    { {
        /* Level gained,  cost,  %fail,  name */
        { 1, 1, 15, _("真見の鏡", "Mirror of Seeing") },
        { 1, 2, 40, _("鏡生成", "Making a Mirror") },
        { 2, 2, 20, _("光のしずく", "Drip of Light") },
        { 3, 2, 20, _("歪んだ鏡", "Warped Mirror") },
        { 5, 3, 35, _("閃光鏡", "Mirror of Light") },

        { 6, 5, 35, _("彷える鏡", "Mirror of Wandering") },
        { 10, 5, 30, _("微塵隠れ", "Robe of Dust") },
        { 12, 12, 30, _("追放の鏡", "Banishing Mirror") },
        { 15, 15, 30, _("鏡砕き", "Mirror Clashing") },
        { 19, 13, 30, _("催眠鏡", "Mirror Sleeping") },

        { 23, 18, 50, _("シーカーレイ", "Seeker Ray") },
        { 25, 20, 40, _("鏡の封印", "Seal of Mirror") },
        { 27, 30, 60, _("水鏡の盾", "Shield of Water") },
        { 29, 30, 60, _("スーパーレイ", "Super Ray") },
        { 31, 35, 60, _("幻惑の光", "Illusion Light") },

        { 33, 50, 80, _("鏡の国", "Mirror Shift") },
        { 36, 30, 80, _("鏡抜け", "Mirror Tunnel") },
        { 38, 40, 70, _("帰還の鏡", "Mirror of Recall") },
        { 40, 50, 55, _("影分身", "Multi-Shadow") },
        { 43, 55, 70, _("封魔結界", "Binding Field") },

        { 46, 70, 75, _("ラフノールの鏡", "Mirror of Ruffnor") },
    } },

    { {
        /* Level gained,  cost,  %fail,  name */
        { 1, 1, 20, _("暗闇生成", "Create Darkness") },
        { 2, 2, 25, _("周辺調査", "Detect Near") },
        { 3, 3, 25, _("葉隠れ", "Hide in Leaves") },
        { 5, 3, 30, _("変わり身", "Kawarimi") },
        { 7, 8, 35, _("高飛び", "Absconding") },

        { 8, 10, 35, _("一撃離脱", "Hit and Away") },
        { 10, 10, 40, _("金縛り", "Bind Monster") },
        { 12, 12, 70, _("古の口伝", "Ancient Knowledge") },
        { 15, 10, 50, _("浮雲", "Floating") },
        { 17, 12, 45, _("火遁", "Hide in Flame") },

        { 18, 20, 40, _("入身", "Nyusin") },
        { 20, 5, 50, _("八方手裏剣", "Syuriken Spreading") },
        { 22, 15, 55, _("鎖鎌", "Chain Hook") },
        { 25, 32, 60, _("煙玉", "Smoke Ball") },
        { 28, 32, 60, _("転身", "Swap Position") },

        { 30, 30, 70, _("爆発の紋章", "Rune of Explosion") },
        { 32, 40, 40, _("土遁", "Hide in Mud") },
        { 34, 35, 50, _("霧隠れ", "Hide in Mist") },
        { 38, 40, 60, _("煉獄火炎", "Rengoku-Kaen") },
        { 41, 50, 55, _("分身", "Bunshin") },
    } },
};

/*! 特殊能力の解説文字列 */
const std::vector<std::vector<std::string>> mind_tips = {
    {
        _("近くの全ての見えるモンスターを感知する。レベル5で罠/"
          "扉、15で透明なモンスター、30で財宝とアイテムを感知できるようになる。レベル20で周辺の地形を感知し、45でその階全体を永久に照らし、"
          "ダンジョン内のすべてのアイテムを感知する。レベル25で一定時間テレパシーを得る。",
            "Detects visible monsters in your vicinity. Detects traps and doors at level 5, invisible monsters at level 15, and items at level 30."
            "Gives telepathy at level 25. Magically maps the surroundings at level 20. Lights and reveals the whole level at level 45."),
        _("精神攻撃のビームまたは球を放つ。", "Fires a beam or ball which inflicts PSI damage."),
        _("近距離のテレポートをする。", "Teleports you a short distance."),
        _("遠距離のテレポートをする。", "Teleports you a long distance."),
        _("レベル30未満で、モンスターを朦朧か混乱か恐怖させる球を放つ。レベル30以上で視界内の全てのモンスターを魅了する。抵抗されると無効。",
            "Stuns, confuses or scares a monster. Or attempts to charm all monsters in sight at level 30."),
        _("テレキネシスの球を放つ。", "Fires a ball which hurts monsters with telekinesis."),
        _("一定時間、ACを上昇させる。レベルが上がると、酸、炎、冷気、電撃、毒の耐性も得られる。",
            "Gives stone skin and some resistance to elements for a while. As your level increases, more resistances are given."),
        _("レベル25未満で、アイテムの雰囲気を知る。レベル25以上で、アイテムを鑑定する。", "Gives feeling of an item. Or identifies an item at level 25."),
        _("レベル25未満で、自分を中心とした精神攻撃の球を発生させる。レベル25以上で、視界内の全てのモンスターに対して精神攻撃を行う。",
            "Generates a ball centered on you which inflicts PSI damage on a monster or, at level 25 and higher, inflicts PSI damage on all monsters."),
        _("恐怖と朦朧から回復し、ヒーロー気分かつ加速状態でなければHPが少し回復する。さらに、一定時間ヒーロー気分になり、加速する。",
            "Removes fear and being stunned. Gives heroism and speed. Heals HP a little unless you already have heroism and a temporary speed boost."),
        _("アイテムを自分の足元へ移動させる。", "Pulls a distant item close to you."),
        _("精神攻撃の球を放つ。モンスターに命中すると、0～1.5ターン消費する。抵抗されなければ、MPが回復する。",
            "Fires a ball which damages. When not resisted, you gain SP. You will be occupied for 0 to 1.5 turns after casting as your mind recovers."),
        _("無傷球をも切り裂く純粋なエネルギーのビームを放つ。", "Fires a beam of pure energy which penetrates invulnerability barriers."),
        _("時を止める。全MPを消費し、消費したMPに応じて長く時を止めていられる。",
            "Stops time. Consumes all of your SP. The more SP consumed, the longer the duration of the spell."),
    },
    {
        _("ごく小さい気の球を放つ。", "Fires a very small energy ball."),
        _("光源が照らしている範囲か部屋全体を永久に明るくする。", "Lights up nearby area and the inside of a room permanently."),
        _("一定時間、空中に浮けるようになる。", "Gives levitation a while."),
        _("射程の短い気のビームを放つ。", "Fires a short energy beam."),
        _("一定時間、魔法防御能力を上昇させる。", "Gives magic resistance for a while."),
        _("気を練る。気を練ると術の威力は上がり、持続時間は長くなる。練った気は時間とともに拡散する。練りすぎると暴走する危険がある。",
            "Increases your spirit energy temporarily. More spirit energy will boost the effect or duration of your force abilities. Too much spirit energy "
            "can result in an explosion."),
        _("一定時間、攻撃してきた全てのモンスターを傷つけるオーラを纏う。",
            "Envelops you with a temporary aura that damages any monster which hits you in melee."),
        _("隣りのモンスターに対して気をぶつけ、吹きとばす。", "Damages an adjacent monster and blows it away."),
        _("大きな気の球を放つ。", "Fires a large energy ball."),
        _("モンスター1体にかかった魔法を解除する。", "Dispels all magics which are affecting a monster."),
        _("1体の幽霊を召喚する。", "Summons ghosts."),
        _("自分を中心とした超巨大な炎の球を発生させる。", "Generates a huge ball of flame centered on you."),
        _("射程の長い、強力な気のビームを放つ。", "Fires a long, powerful energy beam."),
        _("しばらくの間、非常に速く動くことができる。", "Gives extremely fast speed."),
    },
    {
        _("近くの思考することができるモンスターを感知する。", "Detects all monsters except the mindless in your vicinity."),
        _("攻撃した後、反対側に抜ける。",
            "In one action, attacks a monster with your weapons normally and then moves to the space beyond the monster if that space is not blocked."),
        _("トラップにかかるが、そのトラップを破壊する。", "Sets off a trap, then destroys that trap."),
        _("周囲のダンジョンを揺らし、壁と床をランダムに入れ変える。", "Shakes dungeon structure, and results in random swapping of floors and walls."),
        _("全方向に向かって攻撃する。", "Attacks all adjacent monsters."),
    },
    {
        _("近くの全てのモンスターを感知する。レベル15で透明なモンスターを感知する。レベル25で一定時間テレパシーを得る。レベル35で周辺の地形を感知する。"
          "全ての効果は、鏡の上でないとレベル4だけ余計に必要になる。",
            "Detects visible monsters in your vicinity. Detects invisible monsters at level 15. Gives telepathy at level 25. Magically maps the surroundings "
            "at level 35. All of the effects need 4 more levels unless on a mirror."),
        _("自分のいる床の上に鏡を生成する。", "Makes a mirror under you."),
        _("閃光の矢を放つ。レベル10以上では鏡の上で使うとビームになる。",
            "Fires bolt of light. At level ten or higher, the bolt will be a beam of light if you are on a mirror."),
        _("近距離のテレポートをする。", "Teleports you a short distance."),
        _("自分の周囲や、 自分のいる部屋全体を明るくする。", "Lights up nearby area and the inside of a room permanently."),
        _("遠距離のテレポートをする。", "Teleports you a long distance."),
        _("一定時間、鏡のオーラが付く。攻撃を受けると破片のダメージで反撃し、さらに鏡の上にいた場合近距離のテレポートをする。",
            "Gives a temporary aura of mirror shards. The aura damages any monster that hits you in melee."
            " If you are on a mirror, the aura will teleport you a short distance if a monster hits you in melee."),
        _("モンスターをテレポートさせるビームを放つ。抵抗されると無効。", "Teleports all monsters on the line away unless resisted."),
        _("破片の球を放つ。", "Fires a ball of shards."),
        _("全ての鏡の周りに眠りの球を発生させる。", "Causes any mirror to lull to sleep monsters close to the mirror."),
        _("ターゲットに向かって魔力のビームを放つ。鏡に命中すると、その鏡を破壊し、別の鏡に向かって反射する。",
            "Fires a beam of mana. If the beam hits a mirror, it breaks that mirror and bounces toward another mirror."),
        _("鏡の上のモンスターを消し去る。", "Eliminates a monster on a mirror from current dungeon level."),
        _("一定時間、ACを上昇させる。レベル32で反射が付く。レベル40で魔法防御が上がる。",
            "Gives a bonus to AC. Gives reflection at level 32. Gives magic resistance at level 40."),
        _("ターゲットに向かって強力な魔力のビームを放つ。鏡に命中すると、その鏡を破壊し、8方向に魔力のビームを発生させる。",
            "Fires a powerful beam of mana."
            " If the beam hits a mirror, it breaks that mirror and fires 8 beams of mana to 8 different directions from that point."),
        _("視界内のモンスターを減速させ、朦朧とさせ、混乱させ、恐怖させ、麻痺させる。鏡の上で使うと威力が高い。",
            "Attempts to slow, stun, confuse, scare, freeze all monsters in sight. Gets more power on a mirror."),
        _("フロアを作り変える。鏡の上でしか使えない。", "Recreates current dungeon level. Can only be used on a mirror."),
        _("短距離内の指定した場所にテレポートする。", "Teleports you to a given location."),
        _("地上にいるときはダンジョンの最深階へ、ダンジョンにいるときは地上へと移動する。",
            "Recalls player from dungeon to town or from town to the deepest level of dungeon."),
        _("全ての攻撃が、1/2の確率で無効になる。", "Completely protects you from any attacks at one in two chance."),
        _("視界内の2つの鏡とプレイヤーを頂点とする三角形の領域に、魔力の結界を発生させる。",
            "Generates a magical triangle which damages all monsters in the area. The vertices of the triangle are you and two mirrors in sight."),
        _("一定時間、ダメージを受けなくなるバリアを張る。切れた瞬間に少しターンを消費するので注意。",
            "Generates a barrier which completely protects you from almost all damage."
            " Takes a few of your turns when the barrier breaks or duration time is exceeded."),
    },
    {
        _("半径3以内かその部屋を暗くする。", "Darkens nearby area and inside of a room."),
        _("近くの全ての見えるモンスターを感知する。レベル5で罠/扉/"
          "階段、レベル15でアイテムを感知できるようになる。レベル45でその階全体の地形と全てのアイテムを感知する。",
            "Detects visible monsters in your vicinity. Detects traps, doors and stairs at level 5. Detects items at level 15."
            " Lights and reveals the whole level at level 45."),
        _("近距離のテレポートをする。", "Teleports you a short distance."),
        _("攻撃を受けた瞬間にテレポートをするようになる。失敗するとその攻撃のダメージを受ける。テレポートに失敗することもある。",
            "Teleports you as you receive an attack. Might be able to teleport just before receiving damage at higher levels."),
        _("遠距離のテレポートをする。", "Teleports you a long distance."),
        _("攻撃してすぐにテレポートする。", "Attacks an adjacent monster and teleports you away immediately after the attack."),
        _("敵1体の動きを封じる。ユニークモンスター相手の場合又は抵抗された場合には無効。", "Attempts to freeze a monster."),
        _("アイテムを識別する。", "Identifies an item."),
        _("一定時間、浮遊能力を得る。", "Gives levitation for a while."),
        _("自分を中心とした火の球を発生させ、テレポートする。さらに、一定時間炎に対する耐性を得る。装備による耐性に累積する。",
            "Generates a fire ball and immediately teleports you away. Gives resistance to fire for a while."
            " This resistance can be added to that from equipment for more powerful resistance."),
        _("素早く相手に近寄り攻撃する。", "In one action, steps close to a monster and attacks."),
        _("ランダムな方向に8回くさびを投げる。", "Shoots 8 iron Spikes in 8 random directions."),
        _("敵を1体自分の近くに引き寄せる。", "Teleports a monster to a place adjacent to you."),
        _("ダメージのない混乱の球を放つ。", "Releases a confusion ball which doesn't inflict any damage."),
        _("1体のモンスターと位置を交換する。", "Causes you and a targeted monster to exchange positions."),
        _("自分のいる床の上に、モンスターが通ると爆発してダメージを与えるルーンを描く。",
            "Sets a rune under you. The rune will explode when a monster moves on it."),
        _("一定時間、半物質化し壁を通り抜けられるようになる。さらに、一定時間酸への耐性を得る。装備による耐性に累積する。",
            "Makes you ethereal for a period of time. While ethereal, you can pass through walls and are resistant to acid."
            " The resistance can be added to that from equipment for more powerful resistance."),
        _("自分を中心とした超巨大な毒、衰弱、混乱の球を発生させ、テレポートする。",
            "Generates huge balls of poison, drain life and confusion. Then immediately teleports you away."),
        _("ランダムな方向に何回か炎か地獄かプラズマのビームを放つ。", "Fires some number of beams of fire, nether or plasma in random directions."),
        _("全ての攻撃が、1/2の確率で無効になる。",
            "Creates shadows of yourself which gives you the ability to completely evade any attacks at one in two chance for a while."),
    },
};
